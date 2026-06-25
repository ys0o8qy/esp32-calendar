import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
CAP_SYSTEM_SOURCE = ROOT / "components/claw_capabilities/cap_system/src/cap_system.c"
APP_CLAW_SOURCE = ROOT / "components/common/app_claw/app_claw.c"
APP_CLAW_HEADER = ROOT / "components/common/app_claw/include/app_claw.h"
MAIN_SOURCE = ROOT / "application/edge_agent/main/main.c"


class TimeSyncContractTests(unittest.TestCase):
    def test_time_sync_service_refreshes_valid_rtc_time_with_sntp(self):
        source = CAP_SYSTEM_SOURCE.read_text(encoding="utf-8")

        self.assertIn("bool had_valid_time = cap_system_is_time_valid();", source)
        self.assertIn("err = cap_system_sync_time_now(output, sizeof(output));", source)
        self.assertIn("cap_system_notify_time_sync_success(had_valid_time);", source)
        self.assertNotIn("if (cap_system_is_time_valid()) {\n        cap_system_notify_time_sync_success(false);\n        return ESP_OK;\n    }", source)

    def test_app_supplies_network_gate_and_calendar_rtc_sync_callback(self):
        app_source = APP_CLAW_SOURCE.read_text(encoding="utf-8")
        app_header = APP_CLAW_HEADER.read_text(encoding="utf-8")
        main_source = MAIN_SOURCE.read_text(encoding="utf-8")

        self.assertIn("app_claw_set_time_sync_network_ready_callback", app_header)
        self.assertIn("app_claw_set_time_sync_success_callback", app_header)
        self.assertIn(".network_ready = app_claw_time_sync_network_ready", app_source)
        self.assertIn(".on_sync_success = app_claw_time_sync_success", app_source)
        self.assertIn("app_claw_set_time_sync_network_ready_callback(main_time_sync_network_ready, NULL)", main_source)
        self.assertIn("app_claw_set_time_sync_success_callback(main_time_sync_success, NULL)", main_source)
        self.assertIn("calendar_board_data_sync_rtc_from_system_time", main_source)


if __name__ == "__main__":
    unittest.main()
