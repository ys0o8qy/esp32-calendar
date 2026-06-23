import importlib.util
import pathlib
import sys
import tempfile
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "check-render-png.py"


def load_checker():
    spec = importlib.util.spec_from_file_location("check_render_png", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class RenderPngCheckTests(unittest.TestCase):
    def test_rejects_blank_png(self):
        checker = load_checker()
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "blank.png"
            checker.write_test_png(path, 400, 300, lambda _x, _y: (255, 255, 255, 255))

            result = checker.analyze_png(path)

        self.assertFalse(result.ok)
        self.assertIn("blank", result.message)

    def test_accepts_nonblank_calendar_like_png(self):
        checker = load_checker()
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "calendar.png"

            def pixel(x, y):
                if x in (0, 399) or y in (0, 299):
                    return (0, 0, 0, 255)
                if 20 <= x <= 170 and 45 <= y <= 250 and (x % 24 == 0 or y % 32 == 0):
                    return (0, 0, 0, 255)
                if 190 <= x <= 380 and 45 <= y <= 245 and (x % 24 == 0 or y % 24 == 0):
                    return (0, 0, 0, 255)
                return (255, 255, 255, 255)

            checker.write_test_png(path, 400, 300, pixel)

            result = checker.analyze_png(path)

        self.assertTrue(result.ok, result.message)


if __name__ == "__main__":
    unittest.main()
