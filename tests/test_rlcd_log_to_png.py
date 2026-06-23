import importlib.util
import pathlib
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
CHECK_SCRIPT = ROOT / "scripts" / "check-render-png.py"
CONVERT_SCRIPT = ROOT / "scripts" / "rlcd-log-to-png.py"


def load_script(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def set_landscape_pixel(buffer, width, height, x, y, white):
    inv_y = height - 1 - y
    byte_x = x >> 1
    block_y = inv_y >> 2
    blocks_per_column = height >> 2
    index = byte_x * blocks_per_column + block_y
    local_x = x & 1
    local_y = inv_y & 3
    bit = 7 - ((local_y << 1) | local_x)
    if white:
        buffer[index] |= 1 << bit
    else:
        buffer[index] &= ~(1 << bit)


class RlcdLogToPngTests(unittest.TestCase):
    def test_converts_st7305_hex_log_to_png(self):
        converter = load_script("rlcd_log_to_png", CONVERT_SCRIPT)
        checker = load_script("check_render_png_for_rlcd", CHECK_SCRIPT)

        width = 4
        height = 8
        frame = bytearray([0xff] * ((width * height) // 8))
        set_landscape_pixel(frame, width, height, 1, 2, False)

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = pathlib.Path(tmp)
            log = tmp_path / "serial.log"
            png = tmp_path / "frame.png"
            log.write_text(
                "I calendar-display: CALENDAR_RLCD_FRAME_BEGIN width=4 height=8 bytes=4\n"
                f"I calendar-display: CALENDAR_RLCD_FRAME_HEX offset=0 data={frame.hex()}\n"
                "I calendar-display: CALENDAR_RLCD_FRAME_END\n"
            )

            converter.convert_log_to_png(log, png, width, height)
            out_width, out_height, pixels = checker.read_png_rgba(png)

        self.assertEqual((out_width, out_height), (width, height))
        self.assertEqual(pixels[2 * width + 1], (0, 0, 0, 255))
        self.assertEqual(pixels[0], (255, 255, 255, 255))


if __name__ == "__main__":
    unittest.main()
