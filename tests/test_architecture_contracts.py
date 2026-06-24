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

    def test_esp32_platform_initializes_live_time_and_sensor_providers(self):
        platform_source = (ROOT / "src/platform/esp32/calendar_platform.c").read_text(encoding="utf-8")
        kconfig = (ROOT / "main/Kconfig.projbuild").read_text(encoding="utf-8")
        cmake = (ROOT / "main/CMakeLists.txt").read_text(encoding="utf-8")

        self.assertIn("CONFIG_CALENDAR_WIFI_SSID", platform_source)
        self.assertIn("esp_wifi_start", platform_source)
        self.assertIn("esp_netif_sntp_init", platform_source)
        self.assertIn("esp_netif_sntp_sync_wait", platform_source)
        self.assertIn("SHTC3_I2C_ADDRESS", platform_source)
        self.assertIn("i2c_master_transmit_receive", platform_source)
        self.assertIn("CALENDAR_WIFI_SSID", kconfig)
        self.assertIn("CALENDAR_WIFI_PASSWORD", kconfig)
        self.assertIn("CALENDAR_SNTP_SERVER", kconfig)
        self.assertIn("esp_wifi", cmake)
        self.assertIn("esp_netif", cmake)

    def test_shtc3_i2c_timing_matches_waveshare_reference(self):
        platform_source = (ROOT / "src/platform/esp32/calendar_platform.c").read_text(encoding="utf-8")

        self.assertIn("#define SHTC3_WAKE_DELAY_MS 50", platform_source)
        self.assertIn("#define SHTC3_MEASURE_DELAY_MS 20", platform_source)
        self.assertIn("#define CALENDAR_I2C_DATA_TIMEOUT_MS 5000", platform_source)
        self.assertIn("#define CALENDAR_I2C_DONE_TIMEOUT_MS 1000", platform_source)
        self.assertIn("i2c_master_bus_wait_all_done(g_i2c_bus", platform_source)
        self.assertIn("vTaskDelay(pdMS_TO_TICKS(SHTC3_WAKE_DELAY_MS));", platform_source)
        self.assertIn("vTaskDelay(pdMS_TO_TICKS(SHTC3_MEASURE_DELAY_MS));", platform_source)
        self.assertNotIn("pdMS_TO_TICKS(2)", platform_source)

    def test_shtc3_init_probe_failure_keeps_platform_bootable(self):
        platform_source = (ROOT / "src/platform/esp32/calendar_platform.c").read_text(encoding="utf-8")

        self.assertIn("static void calendar_probe_shtc3(void)", platform_source)
        self.assertIn("calendar_probe_shtc3();", platform_source)
        self.assertIn("SHTC3 init probe failed", platform_source)
        self.assertNotIn("ESP_RETURN_ON_ERROR(calendar_shtc3_wake(), TAG, \"SHTC3 wake failed during init\")", platform_source)

    def test_voice_assistant_sdk_is_a_separate_component(self):
        main_source = (ROOT / "main/main.c").read_text(encoding="utf-8")
        cmake = (ROOT / "main/CMakeLists.txt").read_text(encoding="utf-8")
        sdk_header = (ROOT / "components/voice_assistant_sdk/include/voice_assistant.h").read_text(encoding="utf-8")
        sdk_kconfig = (ROOT / "components/voice_assistant_sdk/Kconfig").read_text(encoding="utf-8")

        self.assertIn('#include "voice_assistant.h"', main_source)
        self.assertIn("voice_assistant_start", main_source)
        self.assertIn("voice_assistant_sdk", cmake)
        self.assertIn("config VOICE_ASSISTANT_ENABLE", sdk_kconfig)
        self.assertIn("voice_assistant_register_tool", sdk_header)
        self.assertNotIn("esp_websocket_client", main_source)
        self.assertNotIn("ES8311", main_source)

    def test_dev_verify_renders_after_optional_esp32_build(self):
        script = (ROOT / "scripts/dev-verify.sh").read_text(encoding="utf-8")

        build_position = script.rfind("./scripts/build.sh esp32")
        render_position = script.rfind("./scripts/render-check.sh build-sim/calendar-render.png")

        self.assertGreater(build_position, -1)
        self.assertGreater(render_position, -1)
        self.assertGreater(render_position, build_position)

    def test_project_prefers_lvgl_widgets_before_custom_components(self):
        agents = (ROOT / "AGENTS.md").read_text(encoding="utf-8")
        ui_source = (ROOT / "src/app/calendar_ui.c").read_text(encoding="utf-8")
        model_header = (ROOT / "src/app/calendar_model.h").read_text(encoding="utf-8")
        model_source = (ROOT / "src/app/calendar_model.c").read_text(encoding="utf-8")
        sdkconfig_defaults = (ROOT / "sdkconfig.defaults").read_text(encoding="utf-8")
        sim_conf = (ROOT / "sim/lv_conf.h").read_text(encoding="utf-8")

        self.assertIn("Prefer existing LVGL widgets/components before custom UI components.", agents)
        self.assertIn("lv_calendar_create", ui_source)
        self.assertNotIn("calendar_model_month_grid", model_header)
        self.assertNotIn("calendar_model_month_grid", model_source)
        self.assertIn("CONFIG_LV_USE_BTNMATRIX=y", sdkconfig_defaults)
        self.assertIn("CONFIG_LV_USE_CALENDAR=y", sdkconfig_defaults)
        self.assertIn("#define LV_USE_BTNMATRIX 1", sim_conf)
        self.assertIn("#define LV_USE_CALENDAR 1", sim_conf)

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
