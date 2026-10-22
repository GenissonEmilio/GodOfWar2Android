import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools/desktop"))
from model import Library
from performance import Performance, NVIDIA_ENV, nvidia_offload_available


class PerformanceTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.library = Library(self.root / "data", self.root / "program")
        self.performance = Performance(self.library)
        self.environment = patch.dict(os.environ, {}, clear=True)
        self.environment.start()
        self.addCleanup(self.environment.stop)

    def test_nvidia_requires_secondary_display_device_and_bound_driver(self):
        devices = self.root / "devices"
        device = devices / "0000:01:00.0"
        device.mkdir(parents=True)
        (device / "vendor").write_text("0x10de\n")
        (device / "class").write_text("0x030000\n")
        (device / "boot_vga").write_text("0\n")
        driver = self.root / "drivers/nvidia"
        driver.mkdir(parents=True)
        (device / "driver").symlink_to(driver)
        self.assertTrue(nvidia_offload_available(devices))
        (device / "boot_vga").write_text("1\n")
        self.assertFalse(nvidia_offload_available(devices))
        (device / "boot_vga").write_text("0\n")
        (device / "driver").unlink()
        self.assertFalse(nvidia_offload_available(devices))
        (device / "driver").symlink_to(self.root / "drivers/nouveau")
        self.assertFalse(nvidia_offload_available(devices))
        self.assertFalse(nvidia_offload_available(self.root / "missing"))

    def test_automatic_routes_to_nvidia_but_integrated_uses_native_resolution(self):
        for available, expected_scale in ((True, "2"), (False, "1")):
            with patch("performance.nvidia_offload_available", return_value=available):
                env = self.performance.environment({"PATH": "/usr/bin"})
            self.assertEqual(env["PS2X_GS_HW_SCALE"], expected_scale)
            self.assertEqual(env.get("__NV_PRIME_RENDER_OFFLOAD") == "1", available)
            self.assertEqual(env["PATH"], "/usr/bin")

    def test_user_gpu_and_resolution_overrides_take_priority(self):
        with patch("performance.nvidia_offload_available", return_value=True):
            env = self.performance.environment({"DRI_PRIME": "pci-0000_05_00_0", "PS2X_GS_HW_SCALE": "1.5"})
        self.assertNotIn("__NV_PRIME_RENDER_OFFLOAD", env)
        self.assertEqual(env["DRI_PRIME"], "pci-0000_05_00_0")
        self.assertEqual(env["PS2X_GS_HW_SCALE"], "1.5")
        self.performance.save("system", "3", True)
        with patch("performance.nvidia_offload_available", return_value=True):
            env = self.performance.environment({})
        self.assertNotIn("__NV_PRIME_RENDER_OFFLOAD", env)
        self.assertEqual(env["PS2X_GS_HW_SCALE"], "3")
        self.assertEqual(env["GOW_SHOW_FPS"], "1")

    def test_invalid_preferences_fall_back_and_do_not_touch_display_or_builds(self):
        self.performance.config.parent.mkdir(parents=True)
        for contents in ("[]", "invalid json", '{"gpu":"unknown","scale":99,"fps":"yes"}'):
            self.performance.config.write_text(contents)
            self.assertEqual(self.performance.settings(), {"gpu": "auto", "scale": "auto", "fps": False})
        settings = self.library.player_directory / "gow2_settings.cfg"
        settings.parent.mkdir()
        settings.write_text("fullscreen=1\nscreen=2\n")
        self.performance.save("auto", "1", True)
        self.assertEqual(settings.read_text(), "fullscreen=1\nscreen=2\n")
        self.assertFalse((self.library.directory / "builds").exists())
        self.assertEqual(Performance(self.library).settings()["scale"], "1")

    def test_failed_or_timed_out_driver_probe_falls_back_before_launch(self):
        import subprocess
        for error in (OSError("driver missing"), subprocess.TimeoutExpired("probe", 12)):
            with patch("performance.subprocess.run", side_effect=error):
                env = self.performance.checked_environment(dict(NVIDIA_ENV, PS2X_GS_HW_SCALE="2"))
            self.assertNotIn("__NV_PRIME_RENDER_OFFLOAD", env)
            self.assertNotIn("__GLX_VENDOR_LIBRARY_NAME", env)
            self.assertEqual(env["PS2X_GS_HW_SCALE"], "1")
            self.assertTrue(json.loads((self.library.directory / "last-performance.json").read_text())["fallback"])
        self.performance.save("auto", "3", False)
        with patch("performance.subprocess.run", side_effect=OSError("driver missing")):
            env = self.performance.checked_environment(dict(NVIDIA_ENV, PS2X_GS_HW_SCALE="3"))
        self.assertEqual(env["PS2X_GS_HW_SCALE"], "3")

    def test_successful_probe_keeps_offload_and_external_selection_is_not_probed(self):
        def accepted(args, **kwargs):
            self.assertEqual(kwargs["env"]["__GLX_VENDOR_LIBRARY_NAME"], "nvidia")
            Path(args[-1]).write_text('{"ok":true}')
            return type("Result", (), {"returncode": 0})()
        with patch("performance.subprocess.run", side_effect=accepted):
            env = self.performance.checked_environment(dict(NVIDIA_ENV, PS2X_GS_HW_SCALE="2"))
        self.assertEqual(env["__NV_PRIME_RENDER_OFFLOAD"], "1")
        self.assertFalse(json.loads((self.library.directory / "last-performance.json").read_text())["fallback"])
        with patch.dict(os.environ, NVIDIA_ENV), patch("performance.subprocess.run", side_effect=AssertionError("user override")):
            self.assertEqual(self.performance.checked_environment(env), env)


if __name__ == "__main__":
    unittest.main()
