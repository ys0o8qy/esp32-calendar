# Project Agent Workflow

After any code change or compile/build verification in this repository, run the
RLCD render check before reporting the work as complete.

Required loop:

1. Build or test the changed code.
2. Run:

```bash
./scripts/render-check.sh build-sim/calendar-render.png
```

3. Use `$rlcd-render-check` semantics: inspect `build-sim/calendar-render.png`
   visually after the script passes.
4. Review the PNG from a designer's perspective. Check for:
   - typography problems: inconsistent text sizes, inappropriate hierarchy,
     overly heavy glyphs, weak contrast, or unreadable small text
   - alignment problems: columns, panel edges, calendar cells, baselines, and
     labels not lining up cleanly
   - spacing problems: cramped groups, uneven padding, poor rhythm, or elements
     touching borders
   - overflow problems: clipped text, truncated values, content outside panels,
     or text hidden by fixed-size containers
   - collision problems: text overlap, icon/dot overlap, date number collision,
     or one region visually running into another
   - completeness problems: missing key information, blank regions where content
     should appear, missing glyph boxes, or corrupted CJK text
   - composition problems: unbalanced left/right weight, unclear grouping, or
     panels that compete with the primary time/date focus
5. If any designer-review issue is present, keep iterating: fix the UI, rebuild,
   rerun the render check, and inspect the new image.
6. In the final report, include both the command result and the designer visual
   judgment.

Do not treat a passing structural PNG check as sufficient by itself; the image
must also look visually normal from the checklist above.
