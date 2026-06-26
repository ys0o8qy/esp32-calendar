import pathlib
import subprocess
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
EDGE_AGENT = ROOT / "application/edge_agent"
CALENDAR_HOME = EDGE_AGENT / "components/calendar_home"


class ArchitectureContractTests(unittest.TestCase):
    def test_esp_claw_edge_agent_skeleton_is_the_firmware_entry(self):
        self.assertTrue((EDGE_AGENT / "main/main.c").is_file())
        self.assertTrue((EDGE_AGENT / "main/idf_component.yml").is_file())
        self.assertTrue((ROOT / "components/claw_capabilities").is_dir())
        self.assertTrue((ROOT / "components/claw_modules").is_dir())
        self.assertTrue((ROOT / "components/common").is_dir())
        self.assertTrue((ROOT / "components/lua_modules").is_dir())
        self.assertFalse((ROOT / "main/main.c").exists())
        self.assertFalse((ROOT / "CMakeLists.txt").exists())

    def test_waveshare_rlcd_board_support_is_the_hardware_baseline(self):
        board = EDGE_AGENT / "boards/waveshare/waveshare_ESP32_S3_RLCD_4_2"
        self.assertTrue(board.is_dir())
        self.assertIn("display_lcd", (board / "board_devices.yaml").read_text(encoding="utf-8"))
        self.assertIn("ES8311", (board / "board_devices.yaml").read_text(encoding="utf-8"))
        self.assertIn("ES7210", (board / "board_devices.yaml").read_text(encoding="utf-8"))
        self.assertIn("lcd_width: 400", (board / "board_devices.yaml").read_text(encoding="utf-8"))
        self.assertIn("lcd_height: 300", (board / "board_devices.yaml").read_text(encoding="utf-8"))
        self.assertIn("i2c_master", (board / "board_peripherals.yaml").read_text(encoding="utf-8"))
        self.assertIn("CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT=y", (board / "sdkconfig.defaults.board").read_text(encoding="utf-8"))

    def test_calendar_home_is_the_only_application_level_calendar_entry(self):
        header = (CALENDAR_HOME / "include/calendar_home.h").read_text(encoding="utf-8")
        main_source = (EDGE_AGENT / "main/main.c").read_text(encoding="utf-8")
        manifest = (EDGE_AGENT / "main/idf_component.yml").read_text(encoding="utf-8")

        self.assertIn("esp_err_t calendar_home_start(void);", header)
        self.assertIn('#include "calendar_home.h"', main_source)
        self.assertIn("ESP_ERROR_CHECK(calendar_home_start());", main_source)
        self.assertIn("calendar_home:", manifest)
        self.assertIn("path: ../components/calendar_home", manifest)

    def test_calendar_home_uses_esp_claw_display_arbiter_and_board_display(self):
        home = (CALENDAR_HOME / "src/calendar_home.c").read_text(encoding="utf-8")
        arbiter = (ROOT / "components/common/display_arbiter/display_arbiter.c").read_text(encoding="utf-8")
        arbiter_h = (ROOT / "components/common/display_arbiter/include/display_arbiter.h").read_text(encoding="utf-8")
        lua_runtime = (ROOT / "components/lua_modules/lua_module_lvgl/src/lua_lvgl_runtime.c").read_text(encoding="utf-8")

        self.assertIn("esp_board_manager_get_device_handle(ESP_BOARD_DEVICE_NAME_DISPLAY_LCD", home)
        self.assertIn("esp_lcd_panel_draw_bitmap", home)
        self.assertIn("DISPLAY_ARBITER_OWNER_CALENDAR", arbiter_h)
        self.assertIn("display_arbiter_register_owner_changed_callback", arbiter)
        self.assertIn("display_arbiter_acquire(DISPLAY_ARBITER_OWNER_CALENDAR)", home)
        self.assertIn("owner != DISPLAY_ARBITER_OWNER_CALENDAR", lua_runtime)

    def test_calendar_board_data_reuses_or_falls_back_from_board_manager_i2c(self):
        board_data = (CALENDAR_HOME / "src/calendar_board_data.c").read_text(encoding="utf-8")

        self.assertIn("i2c_master_get_bus_handle", board_data)
        self.assertIn("PCF85063_I2C_ADDRESS", board_data)
        self.assertIn("SHTC3_I2C_ADDRESS", board_data)
        self.assertIn("calendar_bcd_is_valid", board_data)
        self.assertIn("calendar_date_components_valid", board_data)
        self.assertNotIn("esp_wifi_start", board_data)
        self.assertNotIn("esp_netif_sntp_init", board_data)
        self.assertNotIn("CONFIG_CALENDAR_WIFI", board_data)

    def test_calendar_home_reads_wifi_status_into_model_boundary(self):
        home = (CALENDAR_HOME / "src/calendar_home.c").read_text(encoding="utf-8")
        cmake = (CALENDAR_HOME / "CMakeLists.txt").read_text(encoding="utf-8")
        manifest = (CALENDAR_HOME / "idf_component.yml").read_text(encoding="utf-8")

        self.assertIn('#include "wifi_manager.h"', home)
        self.assertIn("wifi_manager_status_t wifi_status", home)
        self.assertIn("wifi_manager_get_status(&wifi_status)", home)
        self.assertIn("model.wifi_connected = wifi_status.sta_connected;", home)
        self.assertIn("wifi_manager", cmake)
        self.assertIn("wifi_manager:", manifest)

    def test_self_built_voice_tts_and_old_rlcd_bridge_are_removed(self):
        removed_paths = [
            "components/voice_assistant_sdk",
            "components/local_tts",
            "src/platform/esp32/calendar_display.c",
            "src/platform/esp32/calendar_display.h",
            "src/platform/esp32/rlcd_mono_buffer.c",
            "src/platform/esp32/rlcd_mono_buffer.h",
            "tests/test_voice_assistant_sdk.c",
            "tests/test_local_tts.c",
            "tests/test_rlcd_mono_buffer.c",
        ]
        for path in removed_paths:
            self.assertFalse((ROOT / path).exists(), path)

    def test_project_prefers_lvgl_widgets_before_custom_components(self):
        agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
        ui_source = (CALENDAR_HOME / "src/calendar_ui.c").read_text(encoding="utf-8")
        model_header = (CALENDAR_HOME / "src/calendar_model.h").read_text(encoding="utf-8")
        model_source = (CALENDAR_HOME / "src/calendar_model.c").read_text(encoding="utf-8")
        sim_conf = (ROOT / "sim/lv_conf.h").read_text(encoding="utf-8")
        defaults = (EDGE_AGENT / "sdkconfig.defaults").read_text(encoding="utf-8")

        self.assertIn("Prefer existing LVGL widgets/components before custom UI components.", agents)
        self.assertIn("lv_calendar_create", ui_source)
        self.assertIn("lv_calendar_get_btnmatrix", ui_source)
        self.assertNotIn("calendar_model_month_grid", model_header)
        self.assertNotIn("calendar_model_month_grid", model_source)
        self.assertIn("#define LV_USE_BTNMATRIX 1", sim_conf)
        self.assertIn("#define LV_USE_CALENDAR 1", sim_conf)
        self.assertIn("CONFIG_LV_USE_BTNMATRIX=y", defaults)
        self.assertIn("CONFIG_LV_USE_CALENDAR=y", defaults)

    def test_simulator_render_chain_is_preserved_against_calendar_home(self):
        sim_cmake = (ROOT / "sim/CMakeLists.txt").read_text(encoding="utf-8")
        sim_source = (ROOT / "sim/main_sdl.c").read_text(encoding="utf-8")
        render_script = (ROOT / "scripts/render-check.sh").read_text(encoding="utf-8")
        dev_verify = (ROOT / "scripts/dev-verify.sh").read_text(encoding="utf-8")
        component_cmake = (CALENDAR_HOME / "CMakeLists.txt").read_text(encoding="utf-8")

        self.assertIn("../application/edge_agent/components/calendar_home/src/calendar_ui.c", sim_cmake)
        self.assertIn("../application/edge_agent/components/calendar_home/src/calendar_font_digits_48.c", sim_cmake)
        self.assertIn("../application/edge_agent/components/calendar_home/src/calendar_font_digits_28.c", sim_cmake)
        self.assertIn('"src/calendar_font_digits_48.c"', component_cmake)
        self.assertIn('"src/calendar_font_digits_28.c"', component_cmake)
        self.assertIn("calendar_model_from_host_time", sim_source)
        self.assertIn("calendar_ui_update(&ui, &model);", sim_source)
        self.assertIn("scripts/check-render-png.py", render_script)
        self.assertIn("./scripts/render-check.sh build-sim/calendar-render.png", dev_verify)

    def test_event_router_run_agent_is_queued_off_router_task_stack(self):
        router = (ROOT / "components/claw_modules/claw_event_router/src/claw_event_router.c").read_text(encoding="utf-8")
        app_claw = (ROOT / "components/common/app_claw/app_claw.c").read_text(encoding="utf-8")
        execute_agent_start = router.index("static esp_err_t claw_event_router_execute_agent_action")
        execute_agent_end = router.index("static esp_err_t claw_event_router_execute_send_message_action")
        execute_agent_body = router[execute_agent_start:execute_agent_end]

        self.assertIn("QueueHandle_t agent_queue", router)
        self.assertIn("TaskHandle_t agent_task_handle", router)
        self.assertIn("claw_event_router_agent_worker_task", router)
        self.assertIn("claw_event_router_enqueue_agent_job", execute_agent_body)
        self.assertNotIn("claw_agent_mgr_submit_root", execute_agent_body)
        self.assertIn(".task_stack_size = 16 * 1024", app_claw)

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
