"""
ctypes wrapper around the vendor WMRAPI Linux libraries (libwmrapi.so).
"""
from __future__ import annotations

import base64
import ctypes
import io
import os
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Optional, Tuple

try:
    from PIL import Image
except ImportError:  # pragma: no cover
    Image = None  # type: ignore


# Error codes from WMRAPI.h
WM_OK = 0
WM_FAIL = -1
WM_INIT_FAIL = -101
WM_FREE_FAIL = -102
WM_OPEN_FAIL = -103
WM_AUTHEN_FAIL = -104
WM_CLOSE_FAIL = -105
WM_GETIMG_FAIL = -106
WM_IMG_TIMEOUT = -107
WM_EXTRACT_FAIL = -110
WM_GENTEMP_FAIL = -111
WM_VERIFY_FAIL = -112
WM_IMGCONVERT_FAIL = -113
WM_FEACONVERT_FAIL = -114
WM_PARAMETER_ERROR = -120

ERROR_NAMES = {
    WM_OK: "OK",
    WM_FAIL: "FAIL",
    WM_INIT_FAIL: "INIT_FAIL",
    WM_FREE_FAIL: "FREE_FAIL",
    WM_OPEN_FAIL: "OPEN_FAIL",
    WM_AUTHEN_FAIL: "AUTHEN_FAIL",
    WM_CLOSE_FAIL: "CLOSE_FAIL",
    WM_GETIMG_FAIL: "GETIMG_FAIL",
    WM_IMG_TIMEOUT: "IMG_TIMEOUT",
    WM_EXTRACT_FAIL: "EXTRACT_FAIL",
    WM_GENTEMP_FAIL: "GENTEMP_FAIL",
    WM_VERIFY_FAIL: "VERIFY_FAIL",
    WM_IMGCONVERT_FAIL: "IMGCONVERT_FAIL",
    WM_FEACONVERT_FAIL: "FEACONVERT_FAIL",
    WM_PARAMETER_ERROR: "PARAMETER_ERROR",
}

LIBS_DIR = Path(__file__).resolve().parent / "libs"
DEFAULT_VERIFY_THRESHOLD = 45  # typical match score threshold used with this SDK


def err_name(code: int) -> str:
    return ERROR_NAMES.get(code, f"UNKNOWN({code})")


class WMRError(RuntimeError):
    def __init__(self, code: int, action: str):
        self.code = code
        self.action = action
        super().__init__(f"{action} failed: {err_name(code)} ({code})")


@dataclass
class CaptureResult:
    raw: bytes
    width: int
    height: int
    bmp: bytes
    preview_png_b64: str


class FingerprintDevice:
    """Thread-safe wrapper for one open WMRAPI session."""

    def __init__(self, libs_dir: Optional[Path] = None):
        self.libs_dir = Path(libs_dir or LIBS_DIR)
        self._lib: Optional[ctypes.CDLL] = None
        self._handle = ctypes.c_size_t(0)
        self._opened = False
        self._initialized = False
        self.width = 0
        self.height = 0
        self.serial = ""
        self._lock = threading.RLock()
        self._has_beep = False
        # Defer dlopen until open() so the HTTP server can start without touching USB.

    def _ensure_loaded(self) -> None:
        if self._lib is None:
            self._load_libraries()

    def _load_libraries(self) -> None:
        # Preload dependency chain the same way the vendor samples do via LD_LIBRARY_PATH.
        # Vendor ships a private libusb1.0.so; load it first with RTLD_GLOBAL.
        prev = os.environ.get("LD_LIBRARY_PATH", "")
        os.environ["LD_LIBRARY_PATH"] = f"{self.libs_dir}:{prev}" if prev else str(self.libs_dir)

        for name in ("libusb1.0.so", "libscsiusb.so", "libfp_sdk.so"):
            path = self.libs_dir / name
            if path.exists():
                ctypes.CDLL(str(path), mode=ctypes.RTLD_GLOBAL)

        lib_path = self.libs_dir / "libwmrapi.so"
        if not lib_path.exists():
            raise FileNotFoundError(f"Missing {lib_path}")

        lib = ctypes.CDLL(str(lib_path), mode=ctypes.RTLD_GLOBAL)

        lib.WM_Init.restype = ctypes.c_int
        lib.WM_Free.restype = ctypes.c_int
        lib.WM_GetDeviceCount.restype = ctypes.c_int

        lib.WM_OpenDevice.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_size_t)]
        lib.WM_OpenDevice.restype = ctypes.c_int

        lib.WM_CloseDevice.argtypes = [ctypes.c_size_t]
        lib.WM_CloseDevice.restype = ctypes.c_int

        lib.WM_GetSerialNumber.argtypes = [ctypes.c_size_t, ctypes.POINTER(ctypes.c_ubyte)]
        lib.WM_GetSerialNumber.restype = ctypes.c_int

        lib.WM_GetImageInfo.argtypes = [ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
        lib.WM_GetImageInfo.restype = ctypes.c_int

        lib.WM_GetImage.argtypes = [
            ctypes.c_size_t,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.WM_GetImage.restype = ctypes.c_int

        lib.WM_Extract.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.WM_Extract.restype = ctypes.c_int

        lib.WM_GenTemplateWithImage3.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.WM_GenTemplateWithImage3.restype = ctypes.c_int

        lib.WM_Verify.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.WM_Verify.restype = ctypes.c_int

        lib.WM_RawToBMP.argtypes = [
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_ubyte),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.WM_RawToBMP.restype = ctypes.c_int

        # WM_Beep exists in the Windows header but is not exported by this Linux .so
        self._has_beep = hasattr(lib, "WM_Beep")
        if self._has_beep:
            lib.WM_Beep.argtypes = [ctypes.c_size_t, ctypes.c_int]
            lib.WM_Beep.restype = ctypes.c_int

        self._lib = lib

    @property
    def lib(self) -> ctypes.CDLL:
        assert self._lib is not None
        return self._lib

    def status(self) -> dict:
        with self._lock:
            count = 0
            if self._initialized:
                try:
                    count = int(self.lib.WM_GetDeviceCount())
                except Exception:
                    count = -1
            return {
                "initialized": self._initialized,
                "opened": self._opened,
                "device_count": count,
                "width": self.width,
                "height": self.height,
                "serial": self.serial,
                "libs_loaded": self._lib is not None,
            }

    def open(self, index: int = 0) -> dict:
        with self._lock:
            if self._opened:
                return self.status()

            self._ensure_loaded()

            ret = self.lib.WM_Init()
            if ret != WM_OK:
                raise WMRError(ret, "WM_Init")
            self._initialized = True

            count = self.lib.WM_GetDeviceCount()
            if count <= 0:
                self.lib.WM_Free()
                self._initialized = False
                raise WMRError(WM_OPEN_FAIL, "WM_GetDeviceCount (no devices)")

            handle = ctypes.c_size_t(0)
            ret = self.lib.WM_OpenDevice(index, ctypes.byref(handle))
            if ret != WM_OK:
                self.lib.WM_Free()
                self._initialized = False
                raise WMRError(ret, "WM_OpenDevice")

            self._handle = handle
            self._opened = True

            w = ctypes.c_int(0)
            h = ctypes.c_int(0)
            ret = self.lib.WM_GetImageInfo(ctypes.byref(w), ctypes.byref(h))
            if ret != WM_OK:
                self.close()
                raise WMRError(ret, "WM_GetImageInfo")
            self.width = w.value
            self.height = h.value

            sn_buf = (ctypes.c_ubyte * 64)()
            ret = self.lib.WM_GetSerialNumber(self._handle, sn_buf)
            if ret == WM_OK:
                self.serial = bytes(sn_buf).split(b"\x00", 1)[0].decode("latin1", errors="ignore")
            else:
                self.serial = ""

            return self.status()

    def close(self) -> None:
        with self._lock:
            if self._opened:
                self.lib.WM_CloseDevice(self._handle)
                self._opened = False
                self._handle = ctypes.c_size_t(0)
            if self._initialized:
                self.lib.WM_Free()
                self._initialized = False
            self.width = 0
            self.height = 0
            self.serial = ""

    def _ensure_open(self) -> None:
        if not self._opened:
            raise RuntimeError("Device is not open. Call /api/open first.")

    def _raw_to_previews(self, raw: bytes) -> Tuple[bytes, str]:
        """Convert RAW image to BMP via SDK, plus optional PNG for the browser."""
        raw_arr = (ctypes.c_ubyte * len(raw)).from_buffer_copy(raw)
        bmp_buf = (ctypes.c_ubyte * (self.width * self.height + 1078 + 64))()
        bmp_size = ctypes.c_int(0)
        ret = self.lib.WM_RawToBMP(
            raw_arr, self.width, self.height, bmp_buf, ctypes.byref(bmp_size)
        )
        if ret != WM_OK:
            # Fallback: synthesize a simple grayscale BMP header-less PNG via Pillow.
            bmp = b""
        else:
            bmp = bytes(bmp_buf[: bmp_size.value])

        png_b64 = ""
        if Image is not None:
            try:
                if bmp.startswith(b"BM"):
                    img = Image.open(io.BytesIO(bmp))
                else:
                    img = Image.frombytes("L", (self.width, self.height), raw)
                out = io.BytesIO()
                img.save(out, format="PNG")
                png_b64 = base64.b64encode(out.getvalue()).decode("ascii")
            except Exception:
                png_b64 = ""
        return bmp, png_b64

    def capture_image(self, timeout_ms: int = 5000, cancel_event: Optional[threading.Event] = None) -> CaptureResult:
        """Block until a fingerprint image is read or cancel_event is set."""
        with self._lock:
            self._ensure_open()
            buf = (ctypes.c_ubyte * (self.width * self.height + 64))()
            size = ctypes.c_int(0)

            while True:
                if cancel_event is not None and cancel_event.is_set():
                    raise RuntimeError("Capture cancelled")

                # Release lock while waiting for USB I/O so status calls can proceed? Keep locked
                # to avoid concurrent USB commands (vendor SDK is not thread-safe).
                ret = self.lib.WM_GetImage(self._handle, timeout_ms, buf, ctypes.byref(size))
                if ret == WM_OK:
                    raw = bytes(buf[: size.value])
                    bmp, png_b64 = self._raw_to_previews(raw)
                    return CaptureResult(
                        raw=raw,
                        width=self.width,
                        height=self.height,
                        bmp=bmp,
                        preview_png_b64=png_b64,
                    )
                if ret == WM_IMG_TIMEOUT:
                    continue
                raise WMRError(ret, "WM_GetImage")

    def enroll_from_raws(self, raw_images: list[bytes]) -> bytes:
        """Build a template from exactly 3 RAW fingerprint images."""
        if len(raw_images) != 3:
            raise ValueError("Enrollment requires exactly 3 fingerprint captures")
        with self._lock:
            self._ensure_open()
            imgs = []
            for raw in raw_images:
                arr = (ctypes.c_ubyte * len(raw)).from_buffer_copy(raw)
                imgs.append(arr)

            template = (ctypes.c_ubyte * 2048)()
            size = ctypes.c_int(0)
            ret = self.lib.WM_GenTemplateWithImage3(
                imgs[0],
                imgs[1],
                imgs[2],
                self.width,
                self.height,
                template,
                ctypes.byref(size),
            )
            if ret != WM_OK:
                raise WMRError(ret, "WM_GenTemplateWithImage3")
            return bytes(template[: size.value])

    def extract_feature(self, raw: bytes) -> bytes:
        with self._lock:
            self._ensure_open()
            image = (ctypes.c_ubyte * len(raw)).from_buffer_copy(raw)
            feature = (ctypes.c_ubyte * 2048)()
            size = ctypes.c_int(0)
            ret = self.lib.WM_Extract(
                image, self.width, self.height, feature, ctypes.byref(size)
            )
            if ret != WM_OK:
                raise WMRError(ret, "WM_Extract")
            return bytes(feature[: size.value])

    def verify(self, template: bytes, feature: bytes) -> Tuple[bool, int]:
        with self._lock:
            tmpl = (ctypes.c_ubyte * len(template)).from_buffer_copy(template)
            feat = (ctypes.c_ubyte * len(feature)).from_buffer_copy(feature)
            score = ctypes.c_int(0)
            ret = self.lib.WM_Verify(tmpl, feat, ctypes.byref(score))
            matched = ret == WM_OK
            return matched, int(score.value)

    def beep(self, on: bool = True) -> None:
        with self._lock:
            if not self._opened or not getattr(self, "_has_beep", False):
                return
            self.lib.WM_Beep(self._handle, 1 if on else 0)


# Lazily constructed so importing the module does not touch native code hard-fail paths.
_device: Optional[FingerprintDevice] = None
_device_lock = threading.Lock()


def get_device() -> FingerprintDevice:
    global _device
    with _device_lock:
        if _device is None:
            _device = FingerprintDevice()
        return _device


# Back-compat attribute for `from wmr_device import device`
class _DeviceProxy:
    def __getattr__(self, name):
        return getattr(get_device(), name)


device = _DeviceProxy()
