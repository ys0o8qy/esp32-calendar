# RLCD Render QA Loop

Use this loop after every code edit and after every compile/build pass. It
prevents structurally valid but visually broken UI from being accepted.

## Trigger

Run the loop after changes to:

- `application/edge_agent/components/calendar_home/` UI, theme, model, board
  data, or font code
- `components/common/display_arbiter/` display ownership code
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
  indoor sensor, RTC/SHTC3 status regions with no obvious typography, spacing,
  clipping, overlap, completeness, or composition problems

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
- Calendar weekday labels, date numbers, highlights, and month headings should
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
- Long status, RTC fallback, sensor, or offline strings should be shortened,
  wrapped deliberately, or clipped only when the loss is intentional.
- Date numbers should not be clipped by the calendar panel.
- Bottom bars and indoor sensor panels must show their complete intended
  content.

### Collision And Overlap

- Text must not overlap other text.
- Calendar highlights must not collide with date numbers.
- Large numeric temperature/time text must not cover labels.
- Adjacent panels must not visually run into each other.

### Completeness And Legibility

- Required information should be visible: current date, time, month grid,
  indoor temperature/humidity, and RTC/SHTC3 status.
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

The migrated firmware uses the ESP-Claw Waveshare board support custom
`display_lcd` panel. The retained canonical QA path is the simulator PNG above;
add a board framebuffer export only if the board support later exposes one.
