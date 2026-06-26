import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
BOARD_DATA_SOURCE = ROOT / "components/calendar_home/src/calendar_board_data.c"
BOARD_DATA_HEADER = ROOT / "components/calendar_home/src/calendar_board_data.h"


class CalendarBoardDataContractTests(unittest.TestCase):
    def test_shtc3_temperature_uses_datasheet_formula_without_fixed_offset(self):
        source = BOARD_DATA_SOURCE.read_text(encoding="utf-8")

        self.assertIn("*temperature_c = 175.0f * (float)raw_temp / 65536.0f - 45.0f;", source)
        self.assertNotIn("- 45.0f -", source)

    def test_rtc_fallback_does_not_mark_rtc_synced_before_trusted_time_sync(self):
        source = BOARD_DATA_SOURCE.read_text(encoding="utf-8")
        header = BOARD_DATA_HEADER.read_text(encoding="utf-8")

        self.assertIn("s_system_time_from_rtc_fallback", source)
        self.assertIn("!s_system_time_from_rtc_fallback", source)
        self.assertIn("calendar_board_data_sync_rtc_from_system_time", source)
        self.assertIn("esp_err_t calendar_board_data_sync_rtc_from_system_time(void);", header)


if __name__ == "__main__":
    unittest.main()
