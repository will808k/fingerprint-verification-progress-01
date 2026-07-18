#!/usr/bin/env bash
# Configure Ubuntu so the Winuim USBKey fingerprint reader (22bc:2009) works
# with the vendor WMRAPI Linux libraries.
#
# Run once with root:
#   sudo bash setup_device.sh
set -euo pipefail

VENDOR_ID="22bc"
PRODUCT_ID="2009"
RULES_FILE="/etc/udev/rules.d/99-winuim-fingerprint.rules"
MODPROBE_FILE="/etc/modprobe.d/winuim-fingerprint.conf"
GROUP_NAME="plugdev"

if [[ "${EUID}" -ne 0 ]]; then
  echo "Please run as root: sudo bash $0"
  exit 1
fi

echo "==> Ensuring group '${GROUP_NAME}' exists"
getent group "${GROUP_NAME}" >/dev/null || groupadd "${GROUP_NAME}"

# Prefer a non-root interactive/login user if available.
TARGET_USER="${SUDO_USER:-${USER:-}}"
if [[ -n "${TARGET_USER}" && "${TARGET_USER}" != "root" ]]; then
  echo "==> Adding user '${TARGET_USER}' to '${GROUP_NAME}'"
  usermod -aG "${GROUP_NAME}" "${TARGET_USER}"
fi

echo "==> Writing udev rule: ${RULES_FILE}"
cat > "${RULES_FILE}" <<EOF
# Winuim / USBKey fingerprint module
# Allow members of plugdev to talk to the device via libusb.
SUBSYSTEM=="usb", ATTR{idVendor}=="${VENDOR_ID}", ATTR{idProduct}=="${PRODUCT_ID}", MODE="0660", GROUP="${GROUP_NAME}", TAG+="uaccess"

# Also cover SCSI generic / optical nodes if the kernel briefly exposes them.
KERNEL=="sg*", ATTRS{idVendor}=="${VENDOR_ID}", ATTRS{idProduct}=="${PRODUCT_ID}", MODE="0660", GROUP="${GROUP_NAME}"
KERNEL=="sr*", ATTRS{idVendor}=="${VENDOR_ID}", ATTRS{idProduct}=="${PRODUCT_ID}", MODE="0660", GROUP="${GROUP_NAME}"
EOF

echo "==> Ignoring device for usb-storage (required so libusb can claim it)"
cat > "${MODPROBE_FILE}" <<EOF
# Prevent the kernel usb-storage driver from claiming the fingerprint reader.
# Without this, the device appears as a fake CD-ROM (sr0) and WMRAPI cannot open it.
options usb-storage quirks=${VENDOR_ID}:${PRODUCT_ID}:i
EOF

echo "==> Reloading udev rules"
udevadm control --reload-rules
udevadm trigger

echo "==> Applying usb-storage quirk to the currently plugged device (if present)"
DEV_PATH=""
for d in /sys/bus/usb/devices/*; do
  if [[ -f "${d}/idVendor" && -f "${d}/idProduct" ]]; then
    if [[ "$(cat "${d}/idVendor")" == "${VENDOR_ID}" && "$(cat "${d}/idProduct")" == "${PRODUCT_ID}" ]]; then
      DEV_PATH="${d}"
      break
    fi
  fi
done

if [[ -n "${DEV_PATH}" ]]; then
  IFACE="$(basename "${DEV_PATH}"):1.0"
  DRIVER_LINK="${DEV_PATH}/1.0/driver"
  # Interface directory is actually <dev>:1.0
  IFACE_DIR="${DEV_PATH}:1.0"
  if [[ -d "${IFACE_DIR}" && -e "${IFACE_DIR}/driver" ]]; then
    CUR_DRIVER="$(basename "$(readlink -f "${IFACE_DIR}/driver")")"
    echo "    Found device at ${DEV_PATH}, interface driver=${CUR_DRIVER}"
    if [[ "${CUR_DRIVER}" == "usb-storage" ]]; then
      echo "    Unbinding usb-storage from ${IFACE} ..."
      echo -n "${IFACE}" > /sys/bus/usb/drivers/usb-storage/unbind || true
      sleep 0.5
    fi
  fi
  # Soft re-enumerate so permissions + quirks take effect
  if [[ -w "${DEV_PATH}/authorized" ]]; then
    echo "    Re-authorizing USB device ..."
    echo 0 > "${DEV_PATH}/authorized"
    sleep 0.5
    echo 1 > "${DEV_PATH}/authorized"
  fi
else
  echo "    Device not currently plugged in (or not found under /sys)."
  echo "    Unplug and replug the reader after setup, or reboot once."
fi

echo
echo "Setup complete."
echo
echo "Next steps:"
echo "  1. Log out/in (or reboot) if you were added to '${GROUP_NAME}'."
echo "  2. Confirm reader is visible:  lsusb | grep -i 22bc"
echo "  3. Confirm usb-storage is NOT bound to the reader interface:"
echo "       for d in /sys/bus/usb/devices/*; do"
echo "         [ -f \"\$d/idVendor\" ] || continue"
echo "         [ \"\$(cat \"\$d/idVendor\")\" = 22bc ] || continue"
echo "         echo \"\$d -> \$(readlink -f \"\$d:1.0/driver\" 2>/dev/null || echo unbound)\""
echo "       done"
echo "  4. Start the demo (root is safest the first time):"
echo "       cd $(cd "$(dirname "$0")" && pwd)"
echo "       sudo bash run_demo.sh"
echo
echo "Vendor note: their Linux C sample also expects root for USB access."
