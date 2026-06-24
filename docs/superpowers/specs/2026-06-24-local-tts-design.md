# Local TTS Design

## Goal

Add a lightweight local text-to-speech capability that runs on the ESP32-S3 target without requiring a network backend. The feature must be a separate module and a reusable voice assistant capability, not application logic embedded in `main.c`.

## Scope

The first version targets short local announcements such as assistant replies, status prompts, weather summaries, and reminders. It prioritizes a small footprint and deterministic PCM generation over natural voice quality. It is not intended to match cloud neural TTS quality.

## Architecture

Create a new `components/local_tts` component with a public `local_tts.h` API. The component owns text tokenization, PCM synthesis, buffer sizing, and engine lifecycle. The default engine is a compact built-in parametric synthesizer that converts UTF-8 text into 16 kHz mono 16-bit PCM. The generated speech is robotic but fully offline and deterministic.

Integrate the component through `voice_assistant_sdk` by adding a `voice_assistant_tts_t` capability to `voice_assistant_config_t`. Callers can invoke `voice_assistant_speak_text(handle, text)`, which synthesizes PCM through the configured TTS engine and plays it through the existing `voice_assistant_audio_port_t.play_pcm` callback. This keeps hardware playback behind the existing ES8311 audio port.

`main.c` configures the capability when `CONFIG_VOICE_ASSISTANT_LOCAL_TTS` is enabled. The UI remains unchanged except that assistant text events can trigger local speech if configured.

## Component Boundaries

- `components/local_tts/include/local_tts.h`: public synthesis API and capability struct.
- `components/local_tts/src/local_tts.c`: compact built-in synthesis implementation.
- `components/voice_assistant_sdk/include/voice_assistant.h`: exposes TTS capability configuration and `voice_assistant_speak_text`.
- `components/voice_assistant_sdk/src/voice_tts.c`: bridges text synthesis to `play_pcm`.
- `main/main.c`: wires the local TTS capability into the assistant configuration.

## Error Handling

Invalid text, missing playback, missing TTS engine, and synthesis buffer overflow return explicit `esp_err_t` values. Playback failures transition the voice assistant to `VOICE_ASSISTANT_STATE_ERROR`. Successful speech emits `SPEAKING` before playback and `IDLE` after playback.

## Testing

Use TDD:

- Add unit tests for local TTS sizing and PCM generation before writing the component.
- Add voice assistant tests for missing capability, successful text playback, event order, and playback failure.
- Run CTest for host C tests, Python unit tests, and the repository render check.

## Render QA

This feature does not alter the LVGL layout, but project policy still requires `./scripts/render-check.sh build-sim/calendar-render.png` after build/test verification. The final report must include the render command result and a visual designer review of the PNG.
