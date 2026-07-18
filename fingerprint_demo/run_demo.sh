#!/usr/bin/env bash
# Launch the fingerprint demo web app with vendor libs on LD_LIBRARY_PATH.
set -euo pipefail
cd "$(dirname "$0")"

export LD_LIBRARY_PATH="$(pwd)/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "Starting demo on http://0.0.0.0:8080 ..."
echo "Tip: if Open device fails, run: sudo bash setup_device.sh"
echo "     then retry as root: sudo bash run_demo.sh"
exec env PYTHONUNBUFFERED=1 python3 -u app.py
