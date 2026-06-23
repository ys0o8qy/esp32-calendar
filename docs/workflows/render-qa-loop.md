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
- designer visual inspection shows readable, well-aligned time, calendar,
  weather, and status regions with no obvious typography, spacing, clipping,
  overlap, completeness, or composition problems

## Designer Review Checklist

Evaluate the exported PNG as a finished 400x300 monochrome interface, not just
as a successful render.

### Typography

- Text sizes should express hierarchy clearly: primary time is dominant, date
  and panel headings are secondary, metadata is quieter.
- Similar text roles should use consistent size and weight.
- Glyphs should not look too heavy, broken, aliased beyond legibility, or
  inconsistent between Chinese and ASCII text.
- Small text must remain readable at the final screenshot size.

### Alignment

- Panel edges should align to an intentional grid.
- Calendar weekday labels, date numbers, event dots, and month headings should
  align consistently.
- Text baselines in the same row should not appear randomly offset.
- Left and right columns should feel deliberately positioned, not accidentally
  drifting.

### Spacing And Rhythm

- Content should not touch panel borders unless intentionally inset.
- Related labels should be grouped; unrelated groups should have visible
  separation.
- Padding should be consistent between panels with similar roles.
- Dense areas should still leave enough white space for scanning.

### Overflow And Clipping

- No text should be cut off by fixed-height containers.
- Long status, weather, event, or offline strings should be shortened, wrapped
  deliberately, or clipped only when the loss is intentional.
- Date numbers should not be clipped by the calendar panel.
- Bottom bars and weather cards must show their complete intended content.

### Collision And Overlap

- Text must not overlap other text.
- Event dots must not collide with date numbers.
- Large numeric temperature/time text must not cover labels.
- Adjacent panels must not visually run into each other.

### Completeness And Legibility

- Required information should be visible: current date, time, month grid,
  weather, next event/offline status.
- There should be no unexpected blank regions where a component failed to draw.
- Missing glyph boxes, corrupted CJK text, or fallback font surprises are
  failures.
- Contrast should make text and borders readable on the RLCD-style monochrome
  output.

### Composition

- The primary focus should remain the time/date, with the month grid as the
  second major focus.
- Left/right visual weight should be balanced enough that the screen does not
  feel lopsided.
- Panels should support scanning, not compete with each other through excessive
  borders, cramped text, or inconsistent shapes.

## Iteration Rule

If either the script or designer review fails, do not stop at reporting the
problem. Fix the layout, font, model text, or render conversion issue, then run
the same loop again. Repeat until the latest exported PNG passes both the
structural check and the designer review checklist.

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
