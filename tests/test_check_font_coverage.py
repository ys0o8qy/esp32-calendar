import importlib.util
import pathlib
import sys
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts" / "check-font-coverage.py"


def load_checker():
    spec = importlib.util.spec_from_file_location("check_font_coverage", SCRIPT)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class CheckFontCoverageTests(unittest.TestCase):
    def test_parses_lv_font_conv_glyph_comments(self):
        checker = load_checker()

        covered = checker.parse_covered_codepoints('/* U+5929 "天" */\n/* U+6C14 "气" */\n')

        self.assertIn(ord("天"), covered)
        self.assertIn(ord("气"), covered)


if __name__ == "__main__":
    unittest.main()
