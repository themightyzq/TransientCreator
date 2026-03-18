# CLAUDE.md — Transient Creator Operations Manual

> This document is a binding contract for AI agent behavior on this project.
> Violations of these rules constitute defects. No exceptions without explicit human approval.

---

## Identity

- **Project:** Transient Creator — JUCE VST3 Audio Plugin
- **Language:** C++17 (minimum), targeting C++20 where JUCE supports it
- **Framework:** JUCE 7.x+ via CMake (NOT Projucer)
- **Build System:** CMake 3.22+
- **Plugin Formats:** VST3, Standalone (AU on macOS when applicable)

---

## Binding Rules

### Code Quality
1. Every function must have a single, clear responsibility. No god-functions.
2. All DSP code must be real-time safe: NO heap allocation, NO locks, NO syscalls, NO exceptions in the audio thread.
3. Use `juce::AudioProcessorValueTreeState` (APVTS) for ALL plugin parameters. No raw member variables for user-facing state.
4. Every parameter must have a unique string ID, a human-readable name, and a defined range with sensible defaults.
5. All audio processing must correctly handle variable buffer sizes and sample rates. `prepareToPlay()` must recalculate all sample-rate-dependent values.
6. Envelope generators must be anti-aliased and produce no discontinuities (clicks/pops) at trigger boundaries.
7. The plugin must be stateless between `processBlock()` calls — all state lives in member variables initialized in `prepareToPlay()`.
8. NEVER use `new`/`delete` in audio code. Use RAII, `std::unique_ptr`, `std::array`, or pre-allocated buffers.
9. Prefer `float` over `double` for all DSP unless precision demands otherwise. Match JUCE's internal float pipeline.
10. All magic numbers must be named constants or constexpr. No unexplained literals in DSP code.

### Architecture
11. Strict separation: PluginProcessor owns DSP. PluginEditor owns UI. They communicate ONLY through APVTS and a lock-free mechanism (e.g., `juce::AbstractFifo` or atomic values) for metering/visualization data.
12. The DSP engine (`TransientEngine`) must be independently testable — no dependency on JUCE's plugin hosting infrastructure.
13. Parameter smoothing must use `juce::SmoothedValue` or equivalent for all parameters that modulate DSP in real-time.
14. Host tempo sync must read from `juce::AudioPlayHead` and gracefully degrade if host provides no tempo information.
15. All UI components must be resizable and DPI-aware.

### Process
16. Before writing ANY code, read the relevant existing files to understand current state. Do not duplicate or contradict existing code.
17. Every change must compile cleanly with zero warnings at `-Wall -Wextra -Wpedantic`.
18. Implement features in the order specified by TODO.md phases. Do not skip ahead.
19. When fixing a bug, write the test FIRST that reproduces it, THEN fix the code.
20. Commit messages follow Conventional Commits: `feat:`, `fix:`, `refactor:`, `docs:`, `test:`, `chore:`.

### What NOT To Do
- Do NOT use the Projucer. This is a CMake-only project.
- Do NOT use `juce::Slider::Listener` pattern. Use APVTS attachments exclusively.
- Do NOT put DSP logic in the Editor class.
- Do NOT use blocking I/O or thread-unsafe operations on the audio thread.
- Do NOT add dependencies without documenting them in README.md and justifying the addition.
- Do NOT use `using namespace juce;` in header files.
- Do NOT create "temporary" hacks. If it ships, it must be correct.

---

## JUCE VST3 Best Practices Integration

The following rules are derived from `JUCE_VST3_BEST_PRACTICES.md` and are binding:

### CMake & Build
- `CMAKE_OSX_ARCHITECTURES` must be set BEFORE `project()` — never after.
- Always build Universal Binary on macOS (arm64 + x86_64).
- Use `JUCE_VST3_CAN_REPLACE_VST2=0` to prevent VST2 replacement conflicts.
- Disable unused features: `JUCE_WEB_BROWSER=0`, `JUCE_USE_CURL=0`.
- Disable Linux-only audio on macOS: `JUCE_JACK=0`, `JUCE_ALSA=0`.
- Use `COPY_PLUGIN_AFTER_BUILD TRUE` for development convenience.

### Plugin Metadata
- `PLUGIN_MANUFACTURER_CODE` = `Zqsf` (4-char, company-wide).
- `PLUGIN_CODE` = `TrCr` (4-char, unique to this plugin).
- `BUNDLE_ID` = `com.zqsfx.transientcreator` (globally unique reverse-domain).
- `EDITOR_WANTS_KEYBOARD_FOCUS FALSE` — allow host shortcuts while UI is open.

### Bus Layout
- Support stereo only (input must match output).
- Constructor must use `BusesProperties().withInput(...stereo...).withOutput(...stereo...)`.
- `isBusesLayoutSupported()` must reject mismatched input/output and non-stereo layouts.

### Real-Time Safety (Audio Thread)
- **Allowed:** Atomic reads/writes, pre-allocated buffer access, SmoothedValue, SIMD.
- **Prohibited:** Memory allocation (`new`/`malloc`), locks/mutexes, file I/O, exceptions, system calls, string operations.
- All buffers must be pre-allocated in `prepareToPlay()`.
- Use `juce::SmoothedValue` for parameter smoothing (20ms default ramp).

### Validation
- Run `pluginval --strictness-level 10` before any release.
- Verify Universal Binary with `file` and `lipo -info` commands.
- Test at 44.1k, 48k, 96k sample rates and buffer sizes 64–2048.
- Manual checklist: loads without crash, audio passes, parameters automate, UI renders, no CPU spikes or leaks.

### Code Signing (Release)
- Sign with `Developer ID Application` certificate using hardened runtime.
- Notarize via `xcrun notarytool submit`.
- Staple the ticket with `xcrun stapler staple`.

---

## Agent Assessment Log

The following agent configuration files were found in `.claude/agents/` and assessed for relevance to this JUCE C++ VST3 transient creation plugin:

| Filename | Decision | Rationale |
|----------|----------|-----------|
| `juce-build-doctor.md` | **KEEP** | Directly relevant — diagnoses JUCE/CMake build issues, plugin loading, code signing. |
| `parameter-auditor.md` | **KEEP** | Directly relevant — traces parameter flow through APVTS to DSP. |
| `audio-qa-tester.md` | **KEEP** | Directly relevant — tests audio playback, parameter controls, UX. |
| `dsp-debugger.md` | **ADJUST** | Relevant for DSP debugging, but references are HPSS/STFT-specific. Useful for general signal flow debugging. |
| `latency-analyzer.md` | **ADJUST** | Relevant for latency reporting, but STFT-specific references need adaptation. Transient Creator has minimal latency. |
| `ci-enhancer.md` | **KEEP** | Relevant for future CI/CD pipeline with pluginval and cross-platform builds. |
| `code-reviewer.md` | **KEEP** | General-purpose code review, applicable to all C++ code. |
| `test-generator.md` | **KEEP** | Useful for generating unit tests for envelope shapes and engine. |
| `ui-polisher.md` | **KEEP** | Useful for Phase 6+ UI polish with JUCE LookAndFeel. |
| `code-refactorer.md` | **KEEP** | General refactoring guidance, useful throughout development. |
| `system-architect.md` | **KEEP** | Useful for architectural decisions as complexity grows. |
| `legacy-cleaner.md` | **KEEP** | Useful for removing dead code during refactoring. |
| `accessibility-pro.md` | **KEEP** | Lower priority, but useful for keyboard navigation in plugin UI. |
| `ux-optimizer.md` | **KEEP** | Useful for simplifying plugin controls and workflow. |
| `stft-validator.md` | **REMOVE** | Not relevant — Transient Creator does not use STFT processing. |
| `hpss-tuner.md` | **REMOVE** | Not relevant — no HPSS algorithm in this project. |
| `izotope-rx-expert.md` | **REMOVE** | Not relevant — iZotope RX guidance has no bearing on this plugin. |

---

## Signal Chain (Current Architecture)

```
Input (External/Noise/Sine) → Envelope × (Intensity + Boost) → Doppler
  → Pre-Delay → HPF → LPF → Dry/Wet Mix → Output Gain
  → [PluginProcessor] → Limiter Compressor → Hard Ceiling Clamp → DAW
```

- Envelope, boost, mix, and gain are per-sample smoothed (SmoothedValue in TransientEngine)
- Filters are block-based (juce::dsp::StateVariableTPTFilter after per-sample loop)
- Limiter is block-based (juce::dsp::Compressor + FloatVectorOperations::clip in PluginProcessor)

---

## Established Patterns (Binding)

- All envelope math constants live in `Source/DSP/EnvelopeConstants.h` — single source of truth
- SmoothedValues for continuous parameters belong in `TransientEngine`, not `PluginProcessor`
- `PluginProcessor` reads atomics and passes raw values to engine setters each processBlock
- Envelope trigger detection uses `consumeTriggerFlag()` pattern, not state comparison
- `recalculateSampleCounts()` uses epsilon caching to skip redundant work
- `recalculateCoefficients()` is factored out for humanize per-trigger recalculation
- The static `computeShapeAtNormalized()` on EnvelopeGenerator is the authoritative visualization method

---

## Parameters (20 total)

| # | ID | Name | Type | Range | Default |
|---|---|------|------|-------|---------|
| 1 | tailLength | Tail Length | Float | 5–5000 ms | 50 ms |
| 2 | silenceGap | Silence Gap | Float | 0–2000 ms | 100 ms |
| 3 | transientShape | Transient Shape | Choice (8) | — | Exponential |
| 4 | intensity | Intensity | Float | 0–100% | 75% |
| 5 | pitchShift | Pitch Shift | Float | 0–24 st | 12 st |
| 6 | mix | Mix | Float | 0–100% | 100% |
| 7 | syncEnabled | Sync to Host | Bool | — | OFF |
| 8 | syncNote | Sync Note Value | Choice (9) | — | 1/4 |
| 9 | inputMode | Input Mode | Choice (4) | — | External |
| 10 | outputGain | Output Gain | Float | -24–+24 dB | 0 dB |
| 11 | limiterOn | Limiter | Bool | — | ON |
| 12 | attackTime | Attack Time | Float | 0–10 ms | 0.1 ms |
| 13 | transientGain | Transient Boost | Float | 0–24 dB | 0 dB |
| 14 | envelopeTension | Curve | Float | 0.1–5.0 | 1.0 |
| 15 | filterHPF | HPF | Float | 20–2000 Hz | 20 Hz |
| 16 | filterLPF | LPF | Float | 200–20000 Hz | 20000 Hz |
| 17 | sineFrequency | Osc Frequency | Float | 20–8000 Hz | 440 Hz |
| 18 | dopplerDirection | Doppler Direction | Choice (3) | — | Recede |
| 19 | preDelay | Pre-Delay | Float | 0–50 ms | 0 ms |
| 20 | humanize | Humanize | Float | 0–100% | 0% |

---

## Task Execution Protocol

When given a task:
1. Read TODO.md to understand current phase and priorities.
2. Read all files that will be touched BEFORE making changes.
3. Plan the implementation. State the plan before coding.
4. Implement with zero shortcuts.
5. Verify the code compiles.
6. Update TODO.md to reflect completed work.
7. If a task reveals new sub-tasks, add them to TODO.md under the appropriate phase.
