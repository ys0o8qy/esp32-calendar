import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
UI_SOURCE = ROOT / "src/app/calendar_ui.c"
THEME_SOURCE = ROOT / "src/app/calendar_theme.c"


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

    def test_weather_metadata_is_split_away_from_large_temperature(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn('snprintf(text, sizeof(text), "湿%d%%", model->humidity_percent);', source)
        self.assertIn('snprintf(text, sizeof(text), "%d-%d°C", model->temp_low_c, model->temp_high_c);', source)
        self.assertNotIn("%d-%dC 湿%d%%", source)

    def test_large_numeric_labels_use_fusion_pixel_font(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("&calendar_font_fusion_48", source)
        self.assertIn("&calendar_font_fusion_28", source)
        self.assertNotIn("lv_font_montserrat", source)

    def test_today_badge_has_opaque_background(self):
        source = THEME_SOURCE.read_text(encoding="utf-8")

        self.assertIn("lv_style_set_bg_opa(&theme->today, LV_OPA_COVER);", source)
        self.assertIn("lv_style_set_pad_hor(&theme->today, 0);", source)
        self.assertIn("lv_style_set_pad_top(&theme->today, 5);", source)
        self.assertIn("lv_style_set_pad_bottom(&theme->today, 0);", source)

    def test_temperature_label_keeps_bottom_padding(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn('snprintf(text, sizeof(text), "%d°C", model->temp_c);', source)
        self.assertIn("make_label_box(weather, text, 62, 20, 72, 34)", source)
        self.assertIn("lv_obj_set_style_text_align(temp, LV_TEXT_ALIGN_RIGHT, 0);", source)

    def test_voice_assistant_status_uses_bottom_right_panel(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("model->assistant_state_text", source)
        self.assertIn("model->assistant_caption", source)
        self.assertIn("model->assistant_error", source)
        self.assertIn('assistant_text = model->assistant_active ? "等待语音结果" : "可按键语音输入";', source)
        self.assertIn("make_bottom_status(ui->screen)", source)
        self.assertIn("lv_obj_set_pos(bar, 10, 264)", source)
        self.assertIn("lv_obj_set_size(bar, 380, 28)", source)
        self.assertNotIn("make_panel(ui->screen, 190, 254, 190, 32)", source)

    def test_top_status_is_short_and_weather_update_stays_with_weather(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn('"Wi-Fi%s · NTP%s · 电%d%%"', source)
        self.assertIn('"更新%s"', source)
        self.assertNotIn('"Wi-Fi%s NTP%s 天气%s 电%d%%"', source)

    def test_calendar_reduces_visual_noise_for_secondary_dates_and_events(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("LV_BORDER_SIDE_BOTTOM", source)
        self.assertIn("draw->rect_dsc->border_width = 2;", source)
        self.assertIn("lv_color_hex(0xffffff)", source)
        self.assertNotIn("draw->rect_dsc->border_width = 1;", source)


if __name__ == "__main__":
    unittest.main()
