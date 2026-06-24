import pathlib
import subprocess
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]


class ArchitectureContractTests(unittest.TestCase):
    def test_firmware_refreshes_display_after_model_updates(self):
        main_source = (ROOT / "main/main.c").read_text(encoding="utf-8")
        display_header = (ROOT / "src/platform/esp32/calendar_display.h").read_text(encoding="utf-8")

        self.assertIn("esp_err_t calendar_display_update(const calendar_model_t *model);", display_header)
        self.assertIn("ESP_ERROR_CHECK(calendar_display_update(&model));", main_source)

    def test_esp32_platform_no_longer_returns_sample_model(self):
        platform_source = (ROOT / "src/platform/esp32/calendar_platform.c").read_text(encoding="utf-8")

        self.assertNotIn("calendar_model_sample()", platform_source)
        self.assertNotIn("will feed calendar_model_t here", platform_source)

    def test_dev_verify_renders_after_optional_esp32_build(self):
        script = (ROOT / "scripts/dev-verify.sh").read_text(encoding="utf-8")

        build_position = script.rfind("./scripts/build.sh esp32")
        render_position = script.rfind("./scripts/render-check.sh build-sim/calendar-render.png")

        self.assertGreater(build_position, -1)
        self.assertGreater(render_position, -1)
        self.assertGreater(render_position, build_position)

    def test_python_bytecode_is_removed_and_ignored(self):
        gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
        result = subprocess.run(
            ["git", "ls-files"],
            cwd=ROOT,
            check=True,
            text=True,
            stdout=subprocess.PIPE,
        )
        tracked_bytecode = [line for line in result.stdout.splitlines() if line.endswith(".pyc") or "__pycache__/" in line]

        self.assertEqual([], tracked_bytecode)
        self.assertIn("*.pyc", gitignore)
        self.assertIn("__pycache__/", gitignore)


if __name__ == "__main__":
    unittest.main()
