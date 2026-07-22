/**
 * koffi wrapper around the vendor WMRAPI Linux libraries (libwmrapi.so).
 * Port of fingerprint_demo/wmr_device.py
 */
"use strict";

const fs = require("fs");
const path = require("path");
const koffi = require("koffi");

const WM_OK = 0;
const WM_OPEN_FAIL = -103;
const WM_IMG_TIMEOUT = -107;

const ERROR_NAMES = {
  [WM_OK]: "OK",
  [-1]: "FAIL",
  [-101]: "INIT_FAIL",
  [-102]: "FREE_FAIL",
  [WM_OPEN_FAIL]: "OPEN_FAIL",
  [-104]: "AUTHEN_FAIL",
  [-105]: "CLOSE_FAIL",
  [-106]: "GETIMG_FAIL",
  [WM_IMG_TIMEOUT]: "IMG_TIMEOUT",
  [-110]: "EXTRACT_FAIL",
  [-111]: "GENTEMP_FAIL",
  [-112]: "VERIFY_FAIL",
  [-113]: "IMGCONVERT_FAIL",
  [-114]: "FEACONVERT_FAIL",
  [-120]: "PARAMETER_ERROR",
};

const DEFAULT_VERIFY_THRESHOLD = 45;
const LIBS_DIR = path.join(__dirname, "libs");

function errName(code) {
  return ERROR_NAMES[code] ?? `UNKNOWN(${code})`;
}

class WMRError extends Error {
  constructor(code, action) {
    super(`${action} failed: ${errName(code)} (${code})`);
    this.code = code;
    this.action = action;
    this.name = "WMRError";
  }
}

/**
 * Simple mutex: vendor SDK is not thread-safe; serialize all calls.
 */
class Mutex {
  constructor() {
    this._locked = false;
    this._queue = [];
  }

  async acquire() {
    if (!this._locked) {
      this._locked = true;
      return;
    }
    await new Promise((resolve) => this._queue.push(resolve));
  }

  release() {
    const next = this._queue.shift();
    if (next) next();
    else this._locked = false;
  }

  async runExclusive(fn) {
    await this.acquire();
    try {
      return await fn();
    } finally {
      this.release();
    }
  }
}

class FingerprintDevice {
  constructor(libsDir = LIBS_DIR) {
    this.libsDir = libsDir;
    this._lib = null;
    this._api = null;
    this._handle = 0n;
    this._opened = false;
    this._initialized = false;
    this.width = 0;
    this.height = 0;
    this.serial = "";
    this._lock = new Mutex();
  }

  _ensureLoaded() {
    if (this._lib) return;

    const prev = process.env.LD_LIBRARY_PATH || "";
    process.env.LD_LIBRARY_PATH = prev
      ? `${this.libsDir}:${prev}`
      : this.libsDir;

    // Preload dependency chain (same order as Python ctypes RTLD_GLOBAL).
    for (const name of ["libusb1.0.so", "libscsiusb.so", "libfp_sdk.so"]) {
      const dep = path.join(this.libsDir, name);
      if (fs.existsSync(dep)) {
        koffi.load(dep);
      }
    }

    const libPath = path.join(this.libsDir, "libwmrapi.so");
    if (!fs.existsSync(libPath)) {
      throw new Error(`Missing ${libPath}`);
    }

    const lib = koffi.load(libPath);

    this._api = {
      WM_Init: lib.func("int WM_Init(void)"),
      WM_Free: lib.func("int WM_Free(void)"),
      WM_GetDeviceCount: lib.func("int WM_GetDeviceCount(void)"),
      WM_OpenDevice: lib.func("int WM_OpenDevice(int nDevIndex, _Out_ size_t *DevHandle)"),
      WM_CloseDevice: lib.func("int WM_CloseDevice(size_t DevHandle)"),
      WM_GetSerialNumber: lib.func(
        "int WM_GetSerialNumber(size_t DevHandle, uint8_t *DeviceSN)"
      ),
      WM_GetImageInfo: lib.func(
        "int WM_GetImageInfo(_Out_ int *ImageWidth, _Out_ int *ImageHeight)"
      ),
      WM_GetImage: lib.func(
        "int WM_GetImage(size_t DevHandle, int nTimeOut, uint8_t *ImageBuf, _Out_ int *Size)"
      ),
      WM_Extract: lib.func(
        "int WM_Extract(uint8_t *ImageBuf, int ImageWidth, int ImageHeight, uint8_t *Feature, _Out_ int *Size)"
      ),
      WM_GenTemplateWithImage3: lib.func(
        "int WM_GenTemplateWithImage3(uint8_t *Image1, uint8_t *Image2, uint8_t *Image3, int nWidth, int nHeight, uint8_t *Template, _Out_ int *Size)"
      ),
      WM_Verify: lib.func(
        "int WM_Verify(uint8_t *Template, uint8_t *Feature, _Out_ int *Score)"
      ),
    };

    this._lib = lib;
  }

  status() {
    return this._lock.runExclusive(() => this._statusUnlocked());
  }

  _statusUnlocked() {
    let count = 0;
    if (this._initialized && this._api) {
      try {
        count = this._api.WM_GetDeviceCount();
      } catch {
        count = -1;
      }
    }
    return {
      initialized: this._initialized,
      opened: this._opened,
      device_count: count,
      width: this.width,
      height: this.height,
      serial: this.serial,
      libs_loaded: this._lib != null,
    };
  }

  async open(index = 0) {
    return this._lock.runExclusive(() => {
      if (this._opened) return this._statusUnlocked();

      this._ensureLoaded();
      const api = this._api;

      let ret = api.WM_Init();
      if (ret !== WM_OK) throw new WMRError(ret, "WM_Init");
      this._initialized = true;

      const count = api.WM_GetDeviceCount();
      if (count <= 0) {
        api.WM_Free();
        this._initialized = false;
        throw new WMRError(WM_OPEN_FAIL, "WM_GetDeviceCount (no devices)");
      }

      const handleOut = [0n];
      ret = api.WM_OpenDevice(index, handleOut);
      if (ret !== WM_OK) {
        api.WM_Free();
        this._initialized = false;
        throw new WMRError(ret, "WM_OpenDevice");
      }

      this._handle = BigInt(handleOut[0]);
      this._opened = true;

      const wOut = [0];
      const hOut = [0];
      ret = api.WM_GetImageInfo(wOut, hOut);
      if (ret !== WM_OK) {
        this._closeUnlocked();
        throw new WMRError(ret, "WM_GetImageInfo");
      }
      this.width = wOut[0];
      this.height = hOut[0];

      const snBuf = Buffer.alloc(64);
      ret = api.WM_GetSerialNumber(this._handle, snBuf);
      if (ret === WM_OK) {
        const nul = snBuf.indexOf(0);
        this.serial = snBuf
          .subarray(0, nul >= 0 ? nul : snBuf.length)
          .toString("latin1");
      } else {
        this.serial = "";
      }

      return this._statusUnlocked();
    });
  }

  async close() {
    return this._lock.runExclusive(() => {
      this._closeUnlocked();
      return { ok: true, ...this._statusUnlocked() };
    });
  }

  _closeUnlocked() {
    if (!this._api) return;
    if (this._opened) {
      this._api.WM_CloseDevice(this._handle);
      this._opened = false;
      this._handle = 0n;
    }
    if (this._initialized) {
      this._api.WM_Free();
      this._initialized = false;
    }
    this.width = 0;
    this.height = 0;
    this.serial = "";
  }

  _ensureOpen() {
    if (!this._opened) {
      throw new Error("Device is not open. Call POST /start first.");
    }
  }

/**
   * Let the HTTP stack flush NDJSON / handle heartbeats before the next
   * blocking vendor SDK call (WM_GetImage holds the Node event loop).
   */
  async _yieldForNetwork() {
    await new Promise((resolve) => setImmediate(resolve));
  }

  /**
   * One non-blocking-to-the-event-loop poll of WM_GetImage.
   * timeoutMs is passed to the SDK (it blocks that long in C).
   */
  async _getImageOnce(timeoutMs) {
    return this._lock.runExclusive(() => {
      this._ensureOpen();
      const api = this._api;
      const buf = Buffer.alloc(this.width * this.height + 64);
      const sizeOut = [0];
      const ret = api.WM_GetImage(this._handle, timeoutMs, buf, sizeOut);
      if (ret === WM_OK) {
        return {
          status: "ok",
          raw: Buffer.from(buf.subarray(0, sizeOut[0])),
          width: this.width,
          height: this.height,
        };
      }
      if (ret === WM_IMG_TIMEOUT) {
        return { status: "timeout" };
      }
      return { status: "error", code: ret };
    });
  }

  /**
   * Block until a fingerprint image is read (retries on IMG_TIMEOUT).
   * Yields between polls so streaming UI events can flush.
   */
  async captureImage(timeoutMs = 5000, overallTimeoutMs = 60000) {
    // Short SDK waits so we can flush network between attempts.
    const pollMs = Math.min(Math.max(Number(timeoutMs) || 5000, 200), 800);
    const deadline = Date.now() + overallTimeoutMs;

    while (true) {
      if (Date.now() > deadline) {
        throw new Error("Capture timed out waiting for finger");
      }
      const result = await this._getImageOnce(pollMs);
      if (result.status === "ok") {
        return {
          raw: result.raw,
          width: result.width,
          height: result.height,
        };
      }
      if (result.status === "error") {
        throw new WMRError(result.code, "WM_GetImage");
      }
      await this._yieldForNetwork();
    }
  }

  /**
   * Wait until the finger is lifted (consecutive image timeouts).
   */
  async waitForFingerLift(options = {}) {
    const timeoutMs = options.timeout_ms ?? 400;
    const neededTimeouts = options.needed_timeouts ?? 2;
    const overallTimeoutMs = options.overall_timeout_ms ?? 30000;
    const pollMs = Math.min(Math.max(Number(timeoutMs) || 400, 150), 500);
    const deadline = Date.now() + overallTimeoutMs;
    let consecutiveTimeouts = 0;

    while (true) {
      if (Date.now() > deadline) {
        throw new Error("Timed out waiting for finger to be lifted");
      }
      const result = await this._getImageOnce(pollMs);
      if (result.status === "timeout") {
        consecutiveTimeouts += 1;
        if (consecutiveTimeouts >= neededTimeouts) return;
      } else if (result.status === "ok") {
        consecutiveTimeouts = 0;
      } else {
        throw new WMRError(result.code, "WM_GetImage");
      }
      await this._yieldForNetwork();
    }
  }

  async enrollFromRaws(rawImages) {
    if (!Array.isArray(rawImages) || rawImages.length !== 3) {
      throw new Error("Enrollment requires exactly 3 fingerprint captures");
    }
    return this._lock.runExclusive(() => {
      this._ensureOpen();
      const api = this._api;
      const template = Buffer.alloc(2048);
      const sizeOut = [0];
      const ret = api.WM_GenTemplateWithImage3(
        rawImages[0],
        rawImages[1],
        rawImages[2],
        this.width,
        this.height,
        template,
        sizeOut
      );
      if (ret !== WM_OK) throw new WMRError(ret, "WM_GenTemplateWithImage3");
      return Buffer.from(template.subarray(0, sizeOut[0]));
    });
  }

  /**
   * Enroll with progress callbacks for UI.
   * onEvent({ event, message, step?, total?, ... })
   * Events: place_finger | captured | lift_finger | processing | complete
   */
  async register(options = {}, onEvent) {
    const emit = async (payload) => {
      if (typeof onEvent === "function") onEvent(payload);
      // Flush NDJSON to the socket before the next blocking SDK poll.
      await this._yieldForNetwork();
      await this._yieldForNetwork();
    };
    const timeoutMs = options.timeout_ms ?? 5000;
    const overallTimeoutMs = options.overall_timeout_ms ?? 90000;
    const total = 3;

    const raws = [];
    for (let step = 1; step <= total; step++) {
      await emit({
        event: "place_finger",
        step,
        total,
        message: `Place finger on the reader (capture ${step} of ${total})`,
      });

      const capt = await this.captureImage(timeoutMs, overallTimeoutMs);
      raws.push(capt.raw);

      await emit({
        event: "captured",
        step,
        total,
        message: `Captured ${step} of ${total}`,
      });

      if (step < total) {
        await emit({
          event: "lift_finger",
          step,
          total,
          message: "Lift your finger, then place again",
        });
        await this.waitForFingerLift({
          timeout_ms: options.lift_timeout_ms ?? 400,
          overall_timeout_ms: options.lift_overall_timeout_ms ?? 30000,
        });
      }
    }

    await emit({
      event: "processing",
      step: total,
      total,
      message: "Building fingerprint template…",
    });

    const template = await this.enrollFromRaws(raws);

    await emit({
      event: "complete",
      ok: true,
      step: total,
      total,
      message: "Registration complete",
      template: template.toString("base64"),
      template_size: template.length,
    });

    return template;
  }

  async extractFeature(raw) {
    return this._lock.runExclusive(() => {
      this._ensureOpen();
      const api = this._api;
      const feature = Buffer.alloc(2048);
      const sizeOut = [0];
      const ret = api.WM_Extract(
        raw,
        this.width,
        this.height,
        feature,
        sizeOut
      );
      if (ret !== WM_OK) throw new WMRError(ret, "WM_Extract");
      return Buffer.from(feature.subarray(0, sizeOut[0]));
    });
  }

  async verify(template, feature) {
    return this._lock.runExclusive(() => {
      const api = this._api;
      const scoreOut = [0];
      const ret = api.WM_Verify(template, feature, scoreOut);
      return { matched: ret === WM_OK, score: scoreOut[0] };
    });
  }

  /**
   * Capture once, extract feature, match against template buffers.
   * Returns the best match that meets the threshold, or null.
   */
  async verifyAgainst(templates, options = {}) {
    const threshold = options.threshold ?? DEFAULT_VERIFY_THRESHOLD;
    const timeoutMs = options.timeout_ms ?? 5000;
    const overallTimeoutMs = options.overall_timeout_ms ?? 60000;

    const capt = await this.captureImage(timeoutMs, overallTimeoutMs);
    const feature = await this.extractFeature(capt.raw);

    let best = null;
    for (const template of templates) {
      const { matched, score } = await this.verify(template, feature);
      if (!matched || score < threshold) continue;
      if (!best || score > best.score) {
        best = { template, score };
      }
    }
    return best;
  }
}

let _device = null;

function getDevice() {
  if (!_device) _device = new FingerprintDevice();
  return _device;
}

module.exports = {
  FingerprintDevice,
  WMRError,
  getDevice,
  DEFAULT_VERIFY_THRESHOLD,
  WM_OK,
  errName,
};
