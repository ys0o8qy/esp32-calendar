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
4. If the image has UI rendering problems, such as overlapping text, missing
   glyphs, clipped content, blank regions, or unreadable layout, keep iterating:
   fix the UI, rebuild, rerun the render check, and inspect the new image.
5. In the final report, include both the command result and the visual judgment.

Do not treat a passing structural PNG check as sufficient by itself; the image
must also look visually normal.
