#!/usr/bin/env python3
"""
Fingerprint enrollment / verification demo web app for Winuim WMRAPI
(reader VID:PID 22bc:2009) on Ubuntu.

Uses only the Python standard library (+ optional Pillow for PNG previews).
"""
from __future__ import annotations

import json
import sqlite3
import threading
import traceback
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse

from wmr_device import DEFAULT_VERIFY_THRESHOLD, WMRError, device, err_name

BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
DB_PATH = DATA_DIR / "fingerprints.db"
HOST = "0.0.0.0"
PORT = 8080

# In-progress enrollment state (simple single-user kiosk demo)
_enroll_lock = threading.Lock()
_enroll_state = {
    "active": False,
    "user_id": "",
    "name": "",
    "captures": [],  # list of raw bytes
    "previews": [],  # list of png b64
    "message": "",
    "error": "",
}
_cancel_event = threading.Event()


def init_db() -> None:
    DATA_DIR.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                template BLOB NOT NULL,
                created_at TEXT DEFAULT (datetime('now'))
            )
            """
        )
        conn.commit()


def list_users() -> list[dict]:
    with sqlite3.connect(DB_PATH) as conn:
        rows = conn.execute(
            "SELECT id, name, length(template) AS tlen, created_at FROM users ORDER BY created_at DESC"
        ).fetchall()
    return [
        {"id": r[0], "name": r[1], "template_bytes": r[2], "created_at": r[3]}
        for r in rows
    ]


def get_user(user_id: str):
    with sqlite3.connect(DB_PATH) as conn:
        return conn.execute(
            "SELECT id, name, template FROM users WHERE id = ?", (user_id,)
        ).fetchone()


def upsert_user(user_id: str, name: str, template: bytes) -> None:
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute(
            """
            INSERT INTO users(id, name, template) VALUES(?,?,?)
            ON CONFLICT(id) DO UPDATE SET name=excluded.name, template=excluded.template,
              created_at=datetime('now')
            """,
            (user_id, name, template),
        )
        conn.commit()


def delete_user(user_id: str) -> bool:
    with sqlite3.connect(DB_PATH) as conn:
        cur = conn.execute("DELETE FROM users WHERE id = ?", (user_id,))
        conn.commit()
        return cur.rowcount > 0


def clear_users() -> None:
    with sqlite3.connect(DB_PATH) as conn:
        conn.execute("DELETE FROM users")
        conn.commit()


def enroll_snapshot() -> dict:
    with _enroll_lock:
        return {
            "active": _enroll_state["active"],
            "user_id": _enroll_state["user_id"],
            "name": _enroll_state["name"],
            "count": len(_enroll_state["captures"]),
            "needed": 3,
            "previews": list(_enroll_state["previews"]),
            "message": _enroll_state["message"],
            "error": _enroll_state["error"],
        }


def json_response(handler: BaseHTTPRequestHandler, code: int, payload: dict) -> None:
    body = json.dumps(payload).encode("utf-8")
    handler.send_response(code)
    handler.send_header("Content-Type", "application/json")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Cache-Control", "no-store")
    handler.end_headers()
    handler.wfile.write(body)


def read_json(handler: BaseHTTPRequestHandler) -> dict:
    length = int(handler.headers.get("Content-Length", "0") or 0)
    if length <= 0:
        return {}
    raw = handler.rfile.read(length)
    if not raw:
        return {}
    return json.loads(raw.decode("utf-8"))


class Handler(BaseHTTPRequestHandler):
    server_version = "FingerprintDemo/1.0"

    def log_message(self, fmt: str, *args) -> None:
        print(f"[{self.log_date_time_string()}] {self.address_string()} {fmt % args}")

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path

        if path in ("/", "/index.html"):
            self._serve_file(BASE_DIR / "templates" / "index.html", "text/html; charset=utf-8")
            return
        if path.startswith("/static/"):
            rel = path[len("/static/") :]
            self._serve_file(BASE_DIR / "static" / rel, self._guess_type(rel))
            return

        if path == "/api/status":
            try:
                st = device.status()
                st["enroll"] = enroll_snapshot()
                st["users"] = list_users()
                json_response(self, 200, {"ok": True, **st})
            except Exception as e:
                json_response(self, 500, {"ok": False, "error": str(e)})
            return

        if path == "/api/users":
            json_response(self, 200, {"ok": True, "users": list_users()})
            return

        if path == "/api/enroll/status":
            json_response(self, 200, {"ok": True, **enroll_snapshot()})
            return

        json_response(self, 404, {"ok": False, "error": "not found"})

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        path = parsed.path
        try:
            data = read_json(self)
        except Exception:
            json_response(self, 400, {"ok": False, "error": "invalid JSON"})
            return

        try:
            if path == "/api/open":
                st = device.open(int(data.get("index", 0)))
                json_response(self, 200, {"ok": True, **st})
                return

            if path == "/api/close":
                device.close()
                json_response(self, 200, {"ok": True, **device.status()})
                return

            if path == "/api/enroll/start":
                user_id = str(data.get("user_id", "")).strip()
                name = str(data.get("name", "")).strip() or user_id
                if not user_id:
                    json_response(self, 400, {"ok": False, "error": "user_id is required"})
                    return
                if not device.status()["opened"]:
                    json_response(self, 400, {"ok": False, "error": "Open the device first"})
                    return
                with _enroll_lock:
                    if _enroll_state["active"]:
                        json_response(self, 409, {"ok": False, "error": "Enrollment already in progress"})
                        return
                    _cancel_event.clear()
                    _enroll_state.update(
                        {
                            "active": True,
                            "user_id": user_id,
                            "name": name,
                            "captures": [],
                            "previews": [],
                            "message": "Place finger on the reader (capture 1 of 3)",
                            "error": "",
                        }
                    )
                threading.Thread(target=_enroll_worker, daemon=True).start()
                json_response(self, 200, {"ok": True, **enroll_snapshot()})
                return

            if path == "/api/enroll/cancel":
                _cancel_event.set()
                with _enroll_lock:
                    _enroll_state["active"] = False
                    _enroll_state["message"] = "Enrollment cancelled"
                json_response(self, 200, {"ok": True, **enroll_snapshot()})
                return

            if path == "/api/verify":
                if not device.status()["opened"]:
                    json_response(self, 400, {"ok": False, "error": "Open the device first"})
                    return
                user_id = str(data.get("user_id", "")).strip()  # empty => 1:N
                threshold = int(data.get("threshold", DEFAULT_VERIFY_THRESHOLD))
                timeout_ms = int(data.get("timeout_ms", 8000))

                # Capture one image then match
                capt = device.capture_image(timeout_ms=timeout_ms)
                feature = device.extract_feature(capt.raw)

                results = []
                best = None
                if user_id:
                    row = get_user(user_id)
                    if not row:
                        json_response(self, 404, {"ok": False, "error": f"Unknown user_id {user_id}"})
                        return
                    matched, score = device.verify(row[2], feature)
                    results.append(
                        {"id": row[0], "name": row[1], "matched": matched, "score": score}
                    )
                    best = results[0]
                else:
                    for uid, uname, tmpl in _all_templates():
                        matched, score = device.verify(tmpl, feature)
                        results.append(
                            {"id": uid, "name": uname, "matched": matched, "score": score}
                        )
                    results.sort(key=lambda r: r["score"], reverse=True)
                    best = results[0] if results else None
                    if best and best["score"] < threshold:
                        best = {**best, "matched": False}

                ok_match = bool(best and best.get("matched") and best.get("score", 0) >= threshold)
                json_response(
                    self,
                    200,
                    {
                        "ok": True,
                        "matched": ok_match,
                        "threshold": threshold,
                        "best": best,
                        "results": results[:10],
                        "preview_png_b64": capt.preview_png_b64,
                    },
                )
                return

            if path == "/api/users/delete":
                user_id = str(data.get("user_id", "")).strip()
                if not user_id:
                    json_response(self, 400, {"ok": False, "error": "user_id required"})
                    return
                json_response(self, 200, {"ok": True, "deleted": delete_user(user_id)})
                return

            if path == "/api/users/clear":
                clear_users()
                json_response(self, 200, {"ok": True})
                return

            json_response(self, 404, {"ok": False, "error": "not found"})
        except WMRError as e:
            json_response(
                self,
                500,
                {"ok": False, "error": str(e), "code": e.code, "code_name": err_name(e.code)},
            )
        except Exception as e:
            traceback.print_exc()
            json_response(self, 500, {"ok": False, "error": str(e)})

    def _serve_file(self, path: Path, content_type: str) -> None:
        if not path.is_file():
            json_response(self, 404, {"ok": False, "error": "file not found"})
            return
        data = path.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(data)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(data)

    @staticmethod
    def _guess_type(name: str) -> str:
        if name.endswith(".css"):
            return "text/css"
        if name.endswith(".js"):
            return "application/javascript"
        if name.endswith(".png"):
            return "image/png"
        if name.endswith(".svg"):
            return "image/svg+xml"
        return "application/octet-stream"


def _all_templates():
    with sqlite3.connect(DB_PATH) as conn:
        return conn.execute("SELECT id, name, template FROM users").fetchall()


def _enroll_worker() -> None:
    try:
        for i in range(3):
            with _enroll_lock:
                if not _enroll_state["active"]:
                    return
                _enroll_state["message"] = f"Place finger on the reader (capture {i + 1} of 3)"
            capt = device.capture_image(timeout_ms=8000, cancel_event=_cancel_event)
            with _enroll_lock:
                if not _enroll_state["active"]:
                    return
                _enroll_state["captures"].append(capt.raw)
                _enroll_state["previews"].append(capt.preview_png_b64)
                _enroll_state["message"] = f"Captured {i + 1} of 3. Lift finger..."
            # brief pause so the same touch isn't re-used immediately
            _cancel_event.wait(1.0)

        with _enroll_lock:
            raws = list(_enroll_state["captures"])
            user_id = _enroll_state["user_id"]
            name = _enroll_state["name"]

        template = device.enroll_from_raws(raws)
        upsert_user(user_id, name, template)

        with _enroll_lock:
            _enroll_state["active"] = False
            _enroll_state["message"] = f"Enrolled user '{user_id}' ({name}) successfully"
            _enroll_state["error"] = ""
    except Exception as e:
        traceback.print_exc()
        with _enroll_lock:
            _enroll_state["active"] = False
            _enroll_state["error"] = str(e)
            _enroll_state["message"] = "Enrollment failed"
            _enroll_state["captures"] = []
            _enroll_state["previews"] = []


def main() -> None:
    import os

    init_db()
    server = ThreadingHTTPServer((HOST, PORT), Handler)
    print(f"Fingerprint demo listening on http://{HOST}:{PORT}")
    print("Open that URL in Chromium on the kiosk.")
    print("Press Ctrl+C to stop.")
    # Auto-open is optional: a failed vendor SDK open can abort the whole process
    # (native segfault). Prefer the UI "Open device" button after setup_device.sh.
    if os.environ.get("FINGERPRINT_AUTO_OPEN", "").lower() in ("1", "true", "yes"):
        try:
            st = device.open()
            print(
                f"Device opened: serial={st.get('serial')} "
                f"size={st.get('width')}x{st.get('height')}"
            )
        except Exception as e:
            print(f"Device not opened at startup ({e}). Use Open device in the UI.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        try:
            device.close()
        except Exception:
            pass
        server.server_close()


if __name__ == "__main__":
    main()
