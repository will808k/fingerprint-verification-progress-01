/**
 * Fingerprint device gateway — Express API for Winuim WMRAPI.
 *
 * GET  /         status
 * POST /start    open device
 * POST /register enroll (NDJSON stream: place/lift/complete + template)
 * POST /verify   match live finger against template(s) from caller
 * POST /stop     close device
 */
"use strict";

const express = require("express");
const {
  getDevice,
  WMRError,
  DEFAULT_VERIFY_THRESHOLD,
} = require("./wmr-device");

const app = express();
const device = getDevice();
const PORT = Number(process.env.PORT || 7000);

// Allow the Next.js UI (and local tools) to stream /register cross-origin.
app.use((req, res, next) => {
  const origin = req.headers.origin;
  if (origin) {
    res.setHeader("Access-Control-Allow-Origin", origin);
    res.setHeader("Vary", "Origin");
  } else {
    res.setHeader("Access-Control-Allow-Origin", "*");
  }
  res.setHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  res.setHeader(
    "Access-Control-Allow-Headers",
    "Content-Type, Accept, Cache-Control"
  );
  res.setHeader("Access-Control-Expose-Headers", "Content-Type");
  if (req.method === "OPTIONS") {
    return res.status(204).end();
  }
  next();
});

// Log as soon as the request arrives (before body parsing).
app.use((req, res, next) => {
  console.log(`[${new Date().toISOString()}] --> ${req.method} ${req.url}`);
  next();
});

/**
 * Parse JSON only when a body is actually present.
 * Clients that send Content-Type: application/json with no body used to hang
 * forever inside express.json(), so /register never ran and nothing was logged.
 */
app.use((req, res, next) => {
  const len = req.headers["content-length"];
  const hasBody = len !== undefined && len !== "0";
  const ct = req.headers["content-type"] || "";
  if (!hasBody || !ct.includes("application/json")) {
    req.body = req.body || {};
    return next();
  }
  express.json({ limit: "2mb" })(req, res, next);
});

function sendError(res, err, fallbackStatus = 500) {
  if (err instanceof WMRError) {
    return res.status(400).json({
      ok: false,
      error: err.message,
      code: err.code,
      action: err.action,
    });
  }
  const msg = err && err.message ? err.message : String(err);
  const status =
    /not open/i.test(msg) || /required/i.test(msg) || /exactly 3/i.test(msg)
      ? 400
      : fallbackStatus;
  return res.status(status).json({ ok: false, error: msg });
}

function decodeTemplate(b64, label = "template") {
  if (typeof b64 !== "string" || !b64.trim()) {
    throw new Error(`${label} must be a non-empty base64 string`);
  }
  const buf = Buffer.from(b64, "base64");
  if (buf.length === 0) {
    throw new Error(`${label} decoded to empty buffer`);
  }
  return buf;
}

function writeNdjson(res, obj) {
  if (res.writableEnded) return;
  const line = `${JSON.stringify(obj)}\n`;
  res.write(line);
  // Push past Node / proxy buffers so the UI gets events immediately.
  if (typeof res.flush === "function") res.flush();
}

/** GET / — device status */
app.get("/", async (_req, res) => {
  try {
    const status = await device.status();
    console.log("<-- GET /", status);
    res.json({ ok: true, ...status });
  } catch (err) {
    sendError(res, err);
  }
});

/** POST /start — open fingerprint device */
app.post("/start", async (req, res) => {
  try {
    const index = Number(req.body?.index ?? 0);
    const status = await device.open(index);
    console.log("<-- POST /start", status);
    res.json({ ok: true, ...status });
  } catch (err) {
    console.error("<-- POST /start error:", err.message || err);
    sendError(res, err);
  }
});

/**
 * POST /register — enroll with streaming progress events (NDJSON).
 *
 * Each line is a JSON object for the UI:
 *   place_finger | captured | lift_finger | processing | complete | error
 *
 * Final success line includes { event: "complete", template, template_size }.
 */
app.post("/register", async (req, res) => {
  console.log("POST /register: checking device…");
  let status;
  try {
    status = await device.status();
  } catch (err) {
    console.error("POST /register: status failed:", err.message || err);
    return sendError(res, err);
  }

  if (!status.opened) {
    console.warn("POST /register: device not open");
    return res
      .status(400)
      .json({ ok: false, error: "Device is not open. Call POST /start first." });
  }

  res.status(200);
  res.setHeader("Content-Type", "application/x-ndjson; charset=utf-8");
  res.setHeader("Cache-Control", "no-cache, no-transform");
  res.setHeader("Connection", "keep-alive");
  res.setHeader("X-Accel-Buffering", "no");
  res.setHeader("X-Content-Type-Options", "nosniff");
  if (res.socket && typeof res.socket.setNoDelay === "function") {
    res.socket.setNoDelay(true);
  }
  if (typeof res.flushHeaders === "function") res.flushHeaders();

  const send = (obj) => {
    console.log("POST /register event:", obj.event, obj.message || "");
    writeNdjson(res, obj);
  };

  // Heartbeat so proxies/clients don't idle-timeout while waiting for a finger.
  const heartbeat = setInterval(() => {
    writeNdjson(res, {
      event: "waiting",
      message: "Still waiting for finger on the reader…",
    });
  }, 10000);

  req.on("close", () => {
    console.log("POST /register: client disconnected");
    clearInterval(heartbeat);
  });

  try {
    console.log("POST /register: enrollment started — place finger when prompted");
    await device.register(
      {
        timeout_ms: req.body?.timeout_ms,
        overall_timeout_ms: req.body?.overall_timeout_ms,
        lift_timeout_ms: req.body?.lift_timeout_ms,
        lift_overall_timeout_ms: req.body?.lift_overall_timeout_ms,
      },
      send
    );
    console.log("POST /register: enrollment finished OK");
  } catch (err) {
    console.error("POST /register error:", err.message || err);
    const payload = {
      event: "error",
      ok: false,
      error: err && err.message ? err.message : String(err),
    };
    if (err instanceof WMRError) {
      payload.code = err.code;
      payload.action = err.action;
    }
    send(payload);
  } finally {
    clearInterval(heartbeat);
  }

  if (!res.writableEnded) res.end();
});

/**
 * POST /verify — capture once and match against templates from external DB.
 *
 * Body: { "templates": ["<base64>", "<base64>", ...] }
 * Optional: threshold, timeout_ms, overall_timeout_ms
 *
 * Match:    { ok: true, matched: true, template, score }
 * No match: { ok: false, matched: false, error: "verification failed" }
 */
app.post("/verify", async (req, res) => {
  try {
    const status = await device.status();
    if (!status.opened) {
      return res
        .status(400)
        .json({ ok: false, error: "Device is not open. Call POST /start first." });
    }

    const rawList = req.body?.templates;
    if (!Array.isArray(rawList) || rawList.length === 0) {
      return res.status(400).json({
        ok: false,
        error: 'Body must include "templates": ["<base64>", ...]',
      });
    }

    const templates = rawList.map((t, i) => {
      if (typeof t !== "string") {
        throw new Error(`templates[${i}] must be a base64 string`);
      }
      return decodeTemplate(t, `templates[${i}]`);
    });

    console.log(`POST /verify: matching against ${templates.length} template(s)`);
    const best = await device.verifyAgainst(templates, {
      threshold: Number(req.body?.threshold ?? DEFAULT_VERIFY_THRESHOLD),
      timeout_ms: req.body?.timeout_ms,
      overall_timeout_ms: req.body?.overall_timeout_ms,
    });

    if (!best) {
      console.log("POST /verify: no match");
      return res.status(200).json({
        ok: false,
        matched: false,
        error: "verification failed",
      });
    }

    console.log("POST /verify: match score", best.score);
    res.json({
      ok: true,
      matched: true,
      template: best.template.toString("base64"),
      score: best.score,
    });
  } catch (err) {
    console.error("POST /verify error:", err.message || err);
    sendError(res, err);
  }
});

/** POST /stop — close device */
app.post("/stop", async (_req, res) => {
  try {
    const result = await device.close();
    console.log("<-- POST /stop");
    res.json(result);
  } catch (err) {
    sendError(res, err);
  }
});

app.use((req, res) => {
  console.warn(`404 ${req.method} ${req.url}`);
  res.status(404).json({ ok: false, error: "Not found" });
});

app.listen(PORT, "0.0.0.0", () => {
  console.log(`Fingerprint API listening on http://0.0.0.0:${PORT}`);
  console.log("Routes: GET /  POST /start  POST /register  POST /verify  POST /stop");
});
