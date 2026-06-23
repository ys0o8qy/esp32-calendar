# RLCD Render QA Loop

Use this loop after every code edit and after every compile/build pass. It
prevents structurally valid but visually broken UI from being accepted.

## Trigger

Run the loop after changes to:

- `src/app/` UI, theme, model, or font code
- `src/platform/esp32/` display or RLCD buffer code
- `sim/` simulator code
- render, font, build, or verification scripts
- any compile/build command before reporting completion

## Commands

For the normal local development path:

```bash
./scripts/dev-verify.sh
```

For only the render part:

```bash
./scripts/render-check.sh build-sim/calendar-render.png
```

For an ESP32 firmware build plus render verification:

```bash
./scripts/dev-verify.sh --esp32
```

## Visual Review

After `render-check.sh` passes, inspect `build-sim/calendar-render.png` using
the `$rlcd-render-check` workflow. The render is acceptable only when both are
true:

- the script reports a valid `400x300` PNG with enough non-background and edge
  detail
- visual inspection shows readable time, calendar, weather, and status regions
  with no major overlap, clipping, blank regions, missing glyph boxes, or
  corrupted CJK text

## Iteration Rule

If either the script or visual review fails, do not stop at reporting the
problem. Fix the layout, font, model text, or render conversion issue, then run
the same loop again. Repeat until the latest exported PNG passes both checks.

## Hardware Frame Path

When validating the board framebuffer, enable `CALENDAR_DUMP_RLCD_FRAME`,
capture the serial log, and convert it:

```bash
python3 scripts/rlcd-log-to-png.py serial.log build-sim/rlcd-frame.png
python3 scripts/check-render-png.py build-sim/rlcd-frame.png
```

Inspect `build-sim/rlcd-frame.png` with the same visual criteria. This verifies
the final pixels sent to the RLCD controller, not physical contrast or refresh
artifacts.
