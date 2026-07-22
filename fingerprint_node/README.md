# Fingerprint Node API (Winuim WMRAPI · 22bc:2009)

Device-gateway HTTP API for enrollment and verification. Templates are **not**
stored here — return them from `/register` and persist them on your external
server; send them back on `/verify`.

## One-time device setup

Same as the Python demo: the reader must not be claimed by `usb-storage`.

```bash
sudo bash ../fingerprint_demo/setup_device.sh
```

Confirm the device is present (`lsusb | grep 22bc`). First tests as `root` are fine.

## Install & run

```bash
cd /home/absa-kampala/testing/fingerprint_node
npm install
bash run.sh
# or: sudo bash run.sh
# PORT=3000 by default
```

`run.sh` sets `LD_LIBRARY_PATH` to `./libs` (symlink to `../fingerprint_demo/libs`).

## Routes

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/` | Reader status (opened, device_count, serial, …) |
| `POST` | `/start` | Open device (`WM_Init` + `WM_OpenDevice`) |
| `POST` | `/register` | Enroll (streams place/lift/complete NDJSON + template) |
| `POST` | `/verify` | Capture once; match against template(s) you supply |
| `POST` | `/stop` | Close device |

### Register

`POST /register` streams **NDJSON** progress lines so the UI can prompt the user
(place / lift / place again). Keep the connection open until `complete` or `error`.

```bash
curl -s -N -X POST http://127.0.0.1:3000/start
curl -s -N -X POST http://127.0.0.1:3000/register
```

Example stream:

```json
{"event":"place_finger","step":1,"total":3,"message":"Place finger on the reader (capture 1 of 3)"}
{"event":"captured","step":1,"total":3,"message":"Captured 1 of 3"}
{"event":"lift_finger","step":1,"total":3,"message":"Lift your finger, then place again"}
{"event":"place_finger","step":2,"total":3,"message":"Place finger on the reader (capture 2 of 3)"}
{"event":"captured","step":2,"total":3,"message":"Captured 2 of 3"}
{"event":"lift_finger","step":2,"total":3,"message":"Lift your finger, then place again"}
{"event":"place_finger","step":3,"total":3,"message":"Place finger on the reader (capture 3 of 3)"}
{"event":"captured","step":3,"total":3,"message":"Captured 3 of 3"}
{"event":"processing","step":3,"total":3,"message":"Building fingerprint template…"}
{"event":"complete","ok":true,"step":3,"total":3,"message":"Registration complete","template":"<base64>","template_size":512}
```

Store `template` from the `complete` event on the external database.

Consumer sketch (browser / Node):

```js
const res = await fetch("http://127.0.0.1:3000/register", { method: "POST" });
const reader = res.body.getReader();
const dec = new TextDecoder();
let buf = "";
while (true) {
  const { value, done } = await reader.read();
  if (done) break;
  buf += dec.decode(value, { stream: true });
  const lines = buf.split("\n");
  buf = lines.pop();
  for (const line of lines) {
    if (!line.trim()) continue;
    const ev = JSON.parse(line);
    // update UI from ev.message / ev.event
    if (ev.event === "complete") console.log("template", ev.template);
  }
}
```

### Verify (external DB)

Matching runs **on this machine** via `WM_Verify`. The remote DB only stores
opaque blobs.

1. Load enrolled templates from your server.
2. `POST /verify` with a string array of those templates.
3. User places a finger once; on success you get the matching template + score
   (threshold default **45**); otherwise `verification failed`.

```bash
curl -s -X POST http://127.0.0.1:3000/verify \
  -H 'Content-Type: application/json' \
  -d '{"templates":["<base64-from-db>","<another-base64>"]}'
```

Match:

```json
{ "ok": true, "matched": true, "template": "<base64>", "score": 72 }
```

No match:

```json
{ "ok": false, "matched": false, "error": "verification failed" }
```

### Stop

```bash
curl -s -X POST http://127.0.0.1:3000/stop
```

## Layout

| Path | Purpose |
|------|---------|
| `server.js` | Express routes |
| `wmr-device.js` | koffi wrapper for `libwmrapi.so` |
| `libs/` | Symlink to vendor `.so` files |
| `run.sh` | `LD_LIBRARY_PATH` + start |

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `/start` fails / segfault | Run `setup_device.sh`, unplug/replug, start as root |
| `device_count` is 0 | Cable / hub / reader not enumerated |
| Register / verify timeouts | Keep finger still; lift between the 3 enroll presses |
