import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
UI_SOURCE = ROOT / "src/app/calendar_ui.c"


class CalendarUiLayoutTests(unittest.TestCase):
    def test_calendar_grid_renders_all_model_rows(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("row < CALENDAR_WEEK_ROWS", source)
        self.assertNotIn("row < 5", source)

    def test_week_number_is_computed_from_model_date(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("calendar_model_iso_week(model->year, model->month, model->day)", source)
        self.assertNotIn('"26周"', source)

    def test_weather_metadata_is_split_away_from_large_temperature(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn('snprintf(text, sizeof(text), "湿%d%%", model->humidity_percent);', source)
        self.assertIn('snprintf(text, sizeof(text), "%d-%dC", model->temp_low_c, model->temp_high_c);', source)
        self.assertNotIn("%d-%dC 湿%d%%", source)

    def test_large_numeric_labels_use_fusion_pixel_font(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("&calendar_font_fusion_48", source)
        self.assertIn("&calendar_font_fusion_28", source)
        self.assertNotIn("lv_font_montserrat", source)


if __name__ == "__main__":
    unittest.main()
