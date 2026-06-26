import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
UI_SOURCE = ROOT / "application/edge_agent/components/calendar_home/src/calendar_ui.c"
KCONFIG_SOURCE = ROOT / "application/edge_agent/components/calendar_home/Kconfig"


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

        self.assertIn('"温度"', source)
        self.assertIn('"湿度"', source)
        self.assertIn('snprintf(temp_text, sizeof(temp_text), "%d°C", model->temp_c);', source)
        self.assertIn('snprintf(humidity_text, sizeof(humidity_text), "%d%%", model->humidity_percent);', source)
        self.assertNotIn('"湿度 %d%%"', source)
        self.assertNotIn('"室内"', source)
        self.assertNotIn("weather_summary", source)
        self.assertNotIn("temp_low_c", source)
        self.assertNotIn("temp_high_c", source)

    def test_temperature_and_humidity_use_large_side_by_side_tiles(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("add_sensor_tile(parent, 18, 172, \"温度\"", source)
        self.assertIn("add_sensor_tile(parent, 102, 172, \"湿度\"", source)
        self.assertIn("lv_obj_set_style_text_font(value, &calendar_font_digits_28, 0)", source)
        self.assertNotIn("add_indoor_panel", source)
        self.assertNotIn('make_label_box(panel, "湿度 --%", 8, 52, 130, 18)', source)

    def test_calendar_home_refreshes_often_enough_for_displayed_clock(self):
        source = KCONFIG_SOURCE.read_text(encoding="utf-8")

        self.assertIn("config CALENDAR_HOME_REFRESH_MS", source)
        self.assertIn("default 1000", source)

    def test_large_numeric_labels_use_bold_digit_font(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("&calendar_font_digits_48", source)
        self.assertIn("&calendar_font_digits_28", source)
        self.assertNotIn("lv_obj_set_style_text_font(time, &calendar_font_fusion_48, 0)", source)
        self.assertNotIn("lv_obj_set_style_text_font(value, &calendar_font_fusion_28, 0)", source)
        self.assertNotIn("lv_font_montserrat", source)

    def test_assistant_status_card_is_present_on_home_screen(self):
        source = UI_SOURCE.read_text(encoding="utf-8")
        model_source = (ROOT / "application/edge_agent/components/calendar_home/src/calendar_model.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("add_assistant_card(ui->screen, model);", source)
        self.assertIn("calendar_model_assistant_state_text(model->assistant_state)", source)
        self.assertIn("model->assistant_detail", source)
        self.assertIn('"待命"', model_source)
        self.assertIn('"麦克风关闭"', source)
        self.assertIn("make_panel(parent, 18, 132, 164, 36)", source)
        self.assertIn("add_status_bar(ui->screen, model);", source)
        self.assertIn("lv_obj_set_pos(bar, 10, 258)", source)
        self.assertIn("lv_obj_set_size(bar, 380, 34)", source)

    def test_todo_placeholder_reserves_right_panel_space_without_data_model(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("add_todo_placeholder(panel);", source)
        self.assertIn('"待办"', source)
        self.assertIn("add_blank_row(panel, 8, 34, 168)", source)
        self.assertIn("add_blank_row(panel, 8, 56, 168)", source)
        self.assertIn("add_blank_row(panel, 8, 78, 168)", source)
        self.assertNotIn("todo_items", source)
        self.assertNotIn("todo_count", source)

    def test_top_status_shows_wifi_battery_and_bottom_status_shows_mic_state(self):
        source = UI_SOURCE.read_text(encoding="utf-8")
        model_source = (ROOT / "application/edge_agent/components/calendar_home/src/calendar_model.c").read_text(
            encoding="utf-8"
        )

        self.assertIn("add_top_status(ui->screen, model);", source)
        self.assertIn('"WiFi %s"', source)
        self.assertIn('model->battery_valid ? "电量 %d%%" : "电量 --"', source)
        self.assertIn("麦克风 %s", model_source)
        self.assertIn('model->mic_muted ? "关闭" : "开启"', model_source)
        self.assertNotIn('model->rtc_fallback_used ? "RTC保时" : "系统时间"', source)
        self.assertNotIn("weather_updated_at", source)

    def test_right_panel_combines_empty_todo_space_and_compact_calendar(self):
        source = UI_SOURCE.read_text(encoding="utf-8")

        self.assertIn("make_panel(parent, 198, 44, 184, 204)", source)
        self.assertIn("add_todo_placeholder(panel);", source)
        self.assertIn("add_compact_month_calendar(panel, model);", source)
        self.assertIn("lv_obj_set_pos(calendar, 8, 112)", source)
        self.assertIn("lv_obj_set_size(calendar, 168, 84)", source)
        self.assertIn("lv_obj_set_size(button_matrix, 168, 82)", source)
        self.assertIn("lv_calendar_create(panel)", source)


if __name__ == "__main__":
    unittest.main()
