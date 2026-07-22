#!/usr/bin/env bash
# Launch the fingerprint Node API with vendor libs on LD_LIBRARY_PATH.
set -euo pipefail
cd "$(dirname "$0")"

export LD_LIBRARY_PATH="$(pwd)/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Match fingerprint-client .env (API_URL / NEXT_PUBLIC_API_URL → :7000).
PORT="${PORT:-7000}"
export PORT

echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "Starting fingerprint API on http://0.0.0.0:${PORT} ..."
echo "Tip: if /start fails, run: sudo bash ../fingerprint_demo/setup_device.sh"
echo "     then retry as root: sudo bash run.sh"
exec node server.js
