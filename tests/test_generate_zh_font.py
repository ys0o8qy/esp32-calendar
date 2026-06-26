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

    def test_scans_calendar_home_text_for_font_coverage(self):
        generator = load_generator()

        self.assertIn(generator.CALENDAR_HOME_SRC / "calendar_board_data.c", generator.SOURCE_PATHS)
        self.assertIn(generator.CALENDAR_HOME_SRC / "calendar_home.c", generator.SOURCE_PATHS)
        self.assertIn(generator.ROOT / "sim/main_sdl.c", generator.SOURCE_PATHS)

    def test_builds_lvgl_font_converter_command_for_calendar_subset(self):
        generator = load_generator()

        cmd = generator.build_lv_font_conv_command(["A", "天"])

        self.assertEqual(cmd[0], "lv_font_conv")
        self.assertIn("--format", cmd)
        self.assertIn("lvgl", cmd)
        self.assertIn("--bpp", cmd)
        self.assertIn("1", cmd)
        self.assertIn("--size", cmd)
        self.assertIn("10", cmd)
        font_args = [cmd[index + 1] for index, value in enumerate(cmd) if value == "--font"]
        self.assertEqual(font_args, ["assets/fonts/fusion-pixel-10px-monospaced-zh_hans.ttf"])
        self.assertFalse(any(pathlib.Path(font_arg).is_absolute() for font_arg in font_args))
        symbol_args = [cmd[index + 1] for index, value in enumerate(cmd) if value == "--symbols"]
        self.assertEqual(symbol_args, ["A天"])
        output_arg = cmd[cmd.index("-o") + 1]
        self.assertEqual(output_arg, "application/edge_agent/components/calendar_home/src/calendar_font_zh.c")
        self.assertFalse(pathlib.Path(output_arg).is_absolute())
        self.assertIn("--lv-font-name", cmd)
        self.assertIn("calendar_font_zh_16", cmd)
        self.assertNotIn("--lv-fallback", cmd)
        self.assertNotIn("lv_font_montserrat_16", cmd)

    def test_uses_fusion_pixel_font_source(self):
        generator = load_generator()

        self.assertEqual(generator.FONT_SOURCE.name, "fusion-pixel-10px-monospaced-zh_hans.ttf")
        self.assertIn("assets/fonts", generator.FONT_SOURCE.as_posix())

    def test_large_temperature_font_includes_degree_symbol(self):
        generator = load_generator()

        variants = {variant["name"]: variant for variant in generator.LARGE_FONT_VARIANTS}
        self.assertIn("°", variants["calendar_font_fusion_28"]["symbols"])
        self.assertIn("%", variants["calendar_font_fusion_28"]["symbols"])

    def test_generates_bold_digit_fonts_for_rlcd_large_numbers(self):
        generator = load_generator()

        variants = {variant["name"]: variant for variant in generator.NUMERIC_FONT_VARIANTS}
        self.assertIn("calendar_font_digits_48", variants)
        self.assertIn("calendar_font_digits_28", variants)
        self.assertEqual(
            variants["calendar_font_digits_48"]["output"].name,
            "calendar_font_digits_48.c",
        )
        self.assertEqual(
            variants["calendar_font_digits_28"]["output"].name,
            "calendar_font_digits_28.c",
        )
        self.assertIn(":", variants["calendar_font_digits_48"]["symbols"])
        self.assertIn("°", variants["calendar_font_digits_28"]["symbols"])
        self.assertIn("%", variants["calendar_font_digits_28"]["symbols"])
        self.assertGreater(variants["calendar_font_digits_48"]["stroke"], 4)
        self.assertGreater(variants["calendar_font_digits_28"]["stroke"], 2)
        self.assertGreaterEqual(variants["calendar_font_digits_28"]["char_widths"]["°"], 9)

    def test_bold_digit_font_renderer_packs_1bpp_glyphs(self):
        generator = load_generator()

        glyph = generator.render_digit_glyph("8", width=22, height=38, stroke=6)
        packed = generator.pack_1bpp_bitmap(glyph)

        self.assertEqual(len(glyph), 38)
        self.assertTrue(all(len(row) == 22 for row in glyph))
        self.assertGreater(sum(sum(row) for row in glyph), 22 * 38 * 0.35)
        self.assertEqual(len(packed), (22 * 38 + 7) // 8)

    def test_bold_digit_font_renderer_draws_unit_symbols_readably(self):
        generator = load_generator()

        percent = generator.render_digit_glyph("%", width=13, height=22, stroke=4)
        degree = generator.render_digit_glyph("°", width=9, height=22, stroke=4)
        rising_diag_hits = sum(
            percent[row][round((len(percent[0]) - 1) * (len(percent) - 1 - row) / (len(percent) - 1))]
            for row in range(len(percent))
        )
        falling_diag_hits = sum(
            percent[row][round((len(percent[0]) - 1) * row / (len(percent) - 1))]
            for row in range(len(percent))
        )

        self.assertGreater(rising_diag_hits, falling_diag_hits)
        self.assertGreater(sum(sum(row) for row in percent[:7]), 14)
        self.assertGreater(sum(sum(row) for row in percent[-7:]), 14)
        self.assertGreater(sum(sum(row) for row in degree[:10]), 18)

    def test_generate_font_runs_lvgl_converter_and_writes_header(self):
        generator = load_generator()

        with mock.patch.object(generator.subprocess, "run") as run:
            with mock.patch.object(generator, "write_numeric_font_file") as write_numeric:
                generator.generate_font(["A", "天"])

        self.assertEqual(run.call_count, 3)
        self.assertEqual(write_numeric.call_count, 2)
        self.assertTrue(all(call.args[0][0] == "lv_font_conv" for call in run.call_args_list))
        self.assertTrue(all(call.kwargs["cwd"] == generator.ROOT for call in run.call_args_list))
        commands = [call.args[0] for call in run.call_args_list]
        outputs = [cmd[cmd.index("-o") + 1] for cmd in commands]
        self.assertEqual(
            outputs,
            [
                "application/edge_agent/components/calendar_home/src/calendar_font_zh.c",
                "application/edge_agent/components/calendar_home/src/calendar_font_fusion_48.c",
                "application/edge_agent/components/calendar_home/src/calendar_font_fusion_28.c",
            ],
        )
        generated_names = [call.args[0]["name"] for call in write_numeric.call_args_list]
        self.assertEqual(generated_names, ["calendar_font_digits_48", "calendar_font_digits_28"])
        self.assertTrue(generator.OUTPUT_H.exists())
        header = generator.OUTPUT_H.read_text(encoding="utf-8")
        self.assertIn("LV_FONT_DECLARE(calendar_font_zh_16);", header)
        self.assertIn("LV_FONT_DECLARE(calendar_font_fusion_48);", header)
        self.assertIn("LV_FONT_DECLARE(calendar_font_fusion_28);", header)
        self.assertIn("LV_FONT_DECLARE(calendar_font_digits_48);", header)
        self.assertIn("LV_FONT_DECLARE(calendar_font_digits_28);", header)


if __name__ == "__main__":
    unittest.main()
