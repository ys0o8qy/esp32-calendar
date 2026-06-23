import pathlib
import re
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
UI_SOURCE = ROOT / "src/app/calendar_ui.c"


class CalendarUiLayoutTests(unittest.TestCase):
    def test_calendar_day_cells_use_compact_numeric_font(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        day_cell_block = re.search(
            r'snprintf\(text, sizeof\(text\), "%d", cell->day\);'
            r"(?P<body>.*?)"
            r"if \(!cell->in_current_month\)",
            source,
            re.DOTALL,
        )
        self.assertIsNotNone(day_cell_block)
        self.assertIn(
            "lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);",
            day_cell_block.group("body"),
        )

    def test_weather_metadata_is_split_away_from_large_temperature(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn('snprintf(text, sizeof(text), "湿%d%%", model->humidity_percent);', source)
        self.assertIn('snprintf(text, sizeof(text), "%d-%dC", model->temp_low_c, model->temp_high_c);', source)
        self.assertNotIn("%d-%dC 湿%d%%", source)


if __name__ == "__main__":
    unittest.main()
