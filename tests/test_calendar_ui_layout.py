import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
UI_SOURCE = ROOT / "application/edge_agent/components/calendar_home/src/calendar_ui.c"


class CalendarUiLayoutTests(unittest.TestCase):
    def test_calendar_panel_uses_lvgl_calendar_widget(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("lv_calendar_create", source)
        self.assertIn("lv_calendar_set_day_names", source)
        self.assertIn("lv_calendar_set_showed_date", source)
        self.assertIn("lv_calendar_set_today_date", source)
        self.assertIn("lv_calendar_set_highlighted_dates", source)
        self.assertIn("lv_calendar_get_btnmatrix", source)
        self.assertNotIn("calendar_model_month_grid", source)
        self.assertNotIn("row < CALENDAR_WEEK_ROWS", source)
        self.assertNotIn("lv_obj_t *dot", source)

    def test_week_number_is_computed_from_model_date(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("calendar_model_iso_week(model->year, model->month, model->day)", source)
        self.assertNotIn('"26周"', source)

    def test_indoor_temperature_and_humidity_are_the_only_weather_data(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn('"室内"', source)
        self.assertIn('"湿度 %d%%"', source)
        self.assertIn('snprintf(text, sizeof(text), "%d°C", model->temp_c);', source)
        self.assertNotIn("weather_summary", source)
        self.assertNotIn("temp_low_c", source)
        self.assertNotIn("temp_high_c", source)

    def test_large_numeric_labels_use_fusion_pixel_font(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("&calendar_font_fusion_48", source)
        self.assertIn("&calendar_font_fusion_28", source)
        self.assertNotIn("lv_font_montserrat", source)

    def test_voice_assistant_status_is_removed_from_home_screen(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertNotIn("assistant_state_text", source)
        self.assertNotIn("assistant_caption", source)
        self.assertNotIn("assistant_error", source)
        self.assertNotIn("等待语音结果", source)
        self.assertIn("add_status_bar(ui->screen, model);", source)
        self.assertIn("lv_obj_set_pos(bar, 10, 258)", source)
        self.assertIn("lv_obj_set_size(bar, 380, 34)", source)

    def test_top_status_is_short_and_not_wifi_battery_driven(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("model->rtc_fallback_used", source)
        self.assertNotIn("Wi-Fi%s", source)
        self.assertNotIn("battery_percent", source)
        self.assertNotIn("weather_updated_at", source)

    def test_calendar_panel_keeps_stable_rlcd_dimensions(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("make_panel(parent, 198, 50, 184, 198)", source)
        self.assertIn("lv_obj_set_size(button_matrix, 168, 152)", source)
        self.assertIn("lv_obj_set_style_text_font(button_matrix, &calendar_font_zh_16, 0)", source)
        self.assertIn("LV_PART_ITEMS | LV_STATE_CHECKED", source)


if __name__ == "__main__":
    unittest.main()
