"""Per-game GPU routing and rendering quality, independent of the build cache."""
from __future__ import annotations
import json
import os
from pathlib import Path
import subprocess
import tempfile

NVIDIA_ENV = {"__NV_PRIME_RENDER_OFFLOAD": "1", "__GLX_VENDOR_LIBRARY_NAME": "nvidia"}
GPU_OVERRIDES = ("DRI_PRIME", "__NV_PRIME_RENDER_OFFLOAD", "__GLX_VENDOR_LIBRARY_NAME",
                 "__NV_PRIME_RENDER_OFFLOAD_PROVIDER")


def nvidia_offload_available(devices=Path("/sys/bus/pci/devices")):
    """Only choose a secondary NVIDIA card with its proprietary driver bound.

    A PCI vendor ID alone is insufficient (nouveau, missing drivers, VMs).
    Reading PCI sysfs also avoids depending on nvidia-smi or switcherooctl.
    """
    try:
        for device in devices.iterdir():
            try:
                if (device / "vendor").read_text().strip() != "0x10de":
                    continue
                if not (device / "class").read_text().strip().startswith("0x03"):
                    continue
                if ((device / "boot_vga").read_text().strip() == "0"
                        and (device / "driver").resolve().name == "nvidia"):
                    return True
            except OSError:
                continue
    except OSError:
        pass
    return False


class Performance:
    def __init__(self, library):
        self.library = library
        self.config = library.directory / "performance.json"

    def settings(self):
        try:
            values = json.loads(self.config.read_text())
            if not isinstance(values, dict):
                values = {}
        except (OSError, ValueError):
            values = {}
        return {"gpu": values.get("gpu") if values.get("gpu") in ("auto", "system") else "auto",
                "scale": values.get("scale") if values.get("scale") in ("auto", "1", "2", "3") else "auto",
                "fps": values.get("fps") is True}

    def save(self, gpu, scale, fps):
        from model import atomic_json
        if gpu not in ("auto", "system") or scale not in ("auto", "1", "2", "3"):
            raise ValueError("Escolha uma GPU e uma resolução interna válidas.")
        atomic_json(self.config, {"gpu": gpu, "scale": scale, "fps": bool(fps)})

    def environment(self, environment):
        result = dict(environment)
        settings = self.settings()
        # A user's PRIME launch options take priority over automatic selection.
        if (settings["gpu"] == "auto" and not any(key in result for key in GPU_OVERRIDES)
                and nvidia_offload_available()):
            result.update(NVIDIA_ENV)
        if "PS2X_GS_HW_SCALE" not in result:
            scale = settings["scale"]
            if scale == "auto":
                scale = "2" if result.get("__NV_PRIME_RENDER_OFFLOAD") == "1" else "1"
            result["PS2X_GS_HW_SCALE"] = scale
        if settings["fps"]:
            result.setdefault("GOW_SHOW_FPS", "1")
        return result

    def checked_environment(self, environment, progress=lambda text: None):
        """Probe offload in a hidden child before starting the game/compiler.

        The launcher never loads a GPU driver into its own GTK process. A failed
        or timed-out driver probe falls back before any game/save is opened.
        """
        result = dict(environment)
        settings = self.settings()
        external_gpu = any(key in os.environ for key in GPU_OVERRIDES)
        automatic_nvidia = (settings["gpu"] == "auto" and not external_gpu
                            and result.get("__NV_PRIME_RENDER_OFFLOAD") == "1")
        report = {"gpu": "external" if external_gpu else "system",
                  "scale": result.get("PS2X_GS_HW_SCALE"), "fallback": False}
        if automatic_nvidia:
            from model import assets_directory
            self.library.directory.mkdir(parents=True, exist_ok=True)
            try:
                with tempfile.TemporaryDirectory(prefix=".gpu-", dir=self.library.directory) as folder:
                    destination = Path(folder) / "probe.json"
                    with (self.library.directory / "last-gpu-probe.log").open("w") as output:
                        probe = subprocess.run([str(self.library.program / "compiler/bin/gow2-video-tool"),
                            "--validate", str(assets_directory() / "shaders/modelo.frag"), "--report", str(destination)],
                            env=result, stdout=output, stderr=subprocess.STDOUT, timeout=12)
                    ok = probe.returncode == 0 and json.loads(destination.read_text()).get("ok") is True
            except (OSError, ValueError, subprocess.TimeoutExpired):
                ok = False
            if ok:
                report["gpu"] = "nvidia-offload"
            else:
                for name in NVIDIA_ENV:
                    result.pop(name, None)
                if settings["scale"] == "auto" and "PS2X_GS_HW_SCALE" not in os.environ:
                    result["PS2X_GS_HW_SCALE"] = "1"
                report.update(gpu="system", scale=result.get("PS2X_GS_HW_SCALE"), fallback=True)
                progress("A GPU dedicada não respondeu. Usando a GPU do sistema…")
        from model import atomic_json
        atomic_json(self.library.directory / "last-performance.json", report)
        return result
