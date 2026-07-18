# Fingerprint demo (Winuim WMRAPI · 22bc:2009)

Ubuntu kiosk demo for enrollment and verification using the vendor Linux libraries
under `fingerprint_files/.../WMRDemo_C_Linux_x64`.

## One-time device setup

The reader enumerates as USB mass-storage ("fake CD-ROM"). The SDK talks to it with
**libusb**, so `usb-storage` must **not** claim the interface.

```bash
cd /home/absa-kampala/testing/fingerprint_demo
sudo bash setup_device.sh
```

That script:

1. Adds a udev rule granting `plugdev` / `uaccess` to `22bc:2009`
2. Sets `usb-storage quirks=22bc:2009:i` so the kernel ignores the device
3. Re-authorizes / unbinds the live device when possible

Then undock sessions or reboot once so group membership applies. First tests as
`root` are fine (vendor C sample also expects root).

Confirm:

```bash
lsusb | grep 22bc
# Driver on the interface should NOT be usb-storage after setup:
for d in /sys/bus/usb/devices/*; do
  [ -f "$d/idVendor" ] || continue
  [ "$(cat "$d/idVendor")" = "22bc" ] || continue
  echo "$d -> $(readlink -f "$d:1.0/driver" 2>/dev/null || echo unbound)"
done
```

## Run the web demo

```bash
cd /home/absa-kampala/testing/fingerprint_demo
bash run_demo.sh
# or: sudo bash run_demo.sh
```

Open **http://127.0.0.1:8080** (or the kiosk LAN IP) in Chromium.

### UI flow

1. **Open device**
2. **Enroll** — enter a User ID, press Start, place the same finger **3 times**
3. **Verify** — leave User ID blank for 1:N, or set it for 1:1; press Capture & verify

Templates are stored in `data/fingerprints.db`.

## Layout

| Path | Purpose |
|------|---------|
| `libs/` | `libwmrapi.so` + deps from vendor Linux sample |
| `wmr_device.py` | ctypes wrapper |
| `app.py` | HTTP API + UI server |
| `setup_device.sh` | udev + usb-storage quirk |
| `run_demo.sh` | starts the app with `LD_LIBRARY_PATH` |

## API (for scripting)

- `POST /api/open` / `POST /api/close`
- `POST /api/enroll/start` `{"user_id":"...","name":"..."}` then poll `GET /api/enroll/status`
- `POST /api/verify` `{"user_id":"","threshold":45}`
- `GET /api/users`

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Open device` fails / segfault | Run `setup_device.sh`, unplug/replug, start demo as root |
| Device count is 0 | Cable / hub / reader not enumerated (`lsusb`) |
| Enrollment / verify timeouts | Keep finger still; lift between the 3 enroll presses |
| No PNG preview | Install `python3-pillow` (`apt install python3-pillow`) |
