import importlib.util
import pathlib
import sys
import unittest
from unittest import mock


ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "generate-zh-font.py"


def load_generator():
    spec = importlib.util.spec_from_file_location("generate_zh_font", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class GenerateZhFontTests(unittest.TestCase):
    def test_extracts_ascii_digits_and_status_letters_for_mixed_labels(self):
        generator = load_generator()

        chars = set(generator.extract_chars())

        for char in "0123456789WiFiNTPRTC:%-C":
            self.assertIn(char, chars)

    def test_builds_lvgl_font_converter_command_for_calendar_subset(self):
        generator = load_generator()

        cmd = generator.build_lv_font_conv_command(["A", "天"])

        self.assertEqual(cmd[0], "lv_font_conv")
        self.assertIn("--format", cmd)
        self.assertIn("lvgl", cmd)
        self.assertIn("--bpp", cmd)
        self.assertIn("1", cmd)
        self.assertIn("--size", cmd)
        self.assertIn("16", cmd)
        self.assertIn("--font", cmd)
        font_arg = cmd[cmd.index("--font") + 1]
        self.assertEqual(font_arg, "third_party/lvgl/scripts/built_in_font/SimSun.woff")
        self.assertFalse(pathlib.Path(font_arg).is_absolute())
        self.assertIn("--symbols", cmd)
        self.assertIn("A天", cmd)
        output_arg = cmd[cmd.index("-o") + 1]
        self.assertEqual(output_arg, "src/app/calendar_font_zh.c")
        self.assertFalse(pathlib.Path(output_arg).is_absolute())
        self.assertIn("--lv-font-name", cmd)
        self.assertIn("calendar_font_zh_16", cmd)
        self.assertIn("--lv-fallback", cmd)
        self.assertIn("lv_font_montserrat_16", cmd)

    def test_uses_lvgl_bundled_woff_font_source_supported_by_converter(self):
        generator = load_generator()

        self.assertEqual(generator.FONT_SOURCE.name, "SimSun.woff")
        self.assertIn("third_party/lvgl/scripts/built_in_font", generator.FONT_SOURCE.as_posix())

    def test_generate_font_runs_lvgl_converter_and_writes_header(self):
        generator = load_generator()

        with mock.patch.object(generator.subprocess, "run") as run:
            generator.generate_font(["A", "天"])

        run.assert_called_once()
        self.assertEqual(run.call_args.args[0][0], "lv_font_conv")
        self.assertEqual(run.call_args.kwargs["cwd"], generator.ROOT)
        self.assertTrue(generator.OUTPUT_H.exists())
        self.assertIn(
            "LV_FONT_DECLARE(calendar_font_zh_16);",
            generator.OUTPUT_H.read_text(encoding="utf-8"),
        )


if __name__ == "__main__":
    unittest.main()
