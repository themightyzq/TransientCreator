# TODO.md — Transient Creator Development Roadmap

> This document tracks all development work. Tasks are organized into sequential phases.
> Do not begin a phase until all critical tasks in the previous phase are complete.
> Update this file after completing any task.

---

## Phase 0: Project Scaffolding
- [x] Audit project directory for existing files, best practices docs, and agent configs
- [x] Read and internalize JUCE_VST3_BEST_PRACTICES.md
- [x] Assess all claude agent/prompt files (keep/adjust/remove) and document in claude.md
- [x] Create project directory structure (Source/, DSP/, UI/, Parameters/, Tests/, Docs/, Assets/)
- [x] Initialize git repository with .gitignore (build/, JUCE/, .DS_Store, *.o, CMake caches)
- [x] Add JUCE as git submodule: `git submodule add https://github.com/juce-framework/JUCE.git JUCE`
- [x] Create root CMakeLists.txt with juce_add_plugin configuration
  - [x] Set PLUGIN_NAME "Transient Creator"
  - [x] Set unique PLUGIN_MANUFACTURER_CODE (Zqsf) and PLUGIN_CODE (TrCr)
  - [x] Configure VST3 and Standalone formats
  - [x] Set IS_SYNTH FALSE, NEEDS_MIDI_INPUT FALSE, IS_MIDI_EFFECT FALSE
  - [x] Set EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  - [x] Link juce::juce_audio_utils, juce::juce_dsp modules
  - [x] Set C++17 standard minimum
  - [x] Enable COPY_PLUGIN_AFTER_BUILD for development
  - [x] Set VST3_CATEGORIES to "Fx|Dynamics|Tools"
- [x] Create .clang-format with project style rules
- [x] Create stub PluginProcessor.h/cpp (inherits juce::AudioProcessor)
- [x] Create stub PluginEditor.h/cpp (inherits juce::AudioProcessorEditor)
- [x] Verify the project builds as an empty passthrough plugin
- [x] Load in a DAW (or JUCE AudioPluginHost) and confirm it instantiates without crashing

---

## Phase 1: Parameter System & Core Architecture
- [x] Design and document all parameter IDs as string constants in Parameters/ParameterLayout.h
- [x] Implement `createParameterLayout()` returning `juce::AudioProcessorValueTreeState::ParameterLayout`
  - [x] Tail Length (ms): float, range 5.0–500.0, default 50.0, skew toward lower values
  - [x] Silence Gap (ms): float, range 0.0–2000.0, default 100.0
  - [x] Transient Shape: choice parameter (Exponential, Linear, Logarithmic, Doppler, ReverseSawtooth, Gaussian, DoubleTap, Percussive)
  - [x] Intensity (%): float, range 0.0–100.0, default 75.0
  - [x] Pitch Shift (semitones): float, range 0.0–24.0, default 12.0 (only active in Doppler mode)
  - [x] Mix (Dry/Wet %): float, range 0.0–100.0, default 100.0
  - [x] Sync to Host: bool, default false
  - [x] Sync Note Value: choice parameter (1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/4T, 1/8T, 1/16T)
  - [x] Input Mode: choice parameter (External Audio, White Noise, Pink Noise, Sine Oscillator)
- [x] Initialize APVTS in PluginProcessor constructor
- [x] Set up parameter listeners or atomic caching for audio-thread-safe reads
- [x] Create SmoothedValue wrappers for all continuously-modulated parameters
- [x] Implement `getStateInformation()` / `setStateInformation()` for preset save/recall
- [ ] Test: Parameters save and restore correctly across plugin reload

---

## Phase 2: Envelope Generator
- [x] Create DSP/EnvelopeGenerator.h with `EnvelopeShape` enum matching parameter choices
- [x] Implement base envelope interface:
  - `void prepare(double sampleRate)`
  - `void trigger()` — starts a new transient cycle
  - `float getNextSample()` — returns envelope amplitude for current sample
  - `bool isActive()` — true if in tail, false if in silence gap
  - `void setTailLength(float ms)`
  - `void setSilenceGap(float ms)`
  - `void setShape(EnvelopeShape shape)`
- [x] Implement Exponential Decay: `amplitude = exp(-t * decayRate)` where decayRate is derived from tail length
- [x] Implement Linear Decay: `amplitude = 1.0 - (t / tailSamples)`
- [x] Implement Logarithmic Decay: `amplitude = 1.0 - log(1 + t * k) / log(1 + tailSamples * k)`
- [x] Implement Reverse Sawtooth: instant max, linear drop, hard cutoff at tail end
- [x] Implement Gaussian Pulse: `amplitude = exp(-0.5 * ((t - center) / sigma)^2)` centered at start
- [x] Implement Double Tap: two exponential pulses with configurable inter-tap spacing
- [x] Implement Percussive: fast exponential attack (~1ms), short resonant body, exponential decay
- [x] Ensure ALL envelopes produce sample-accurate zero-crossings at boundaries (no clicks)
- [x] Anti-aliasing: apply short crossfade (~32 samples) at transient re-trigger boundaries
- [ ] Write unit tests for each envelope shape (verify amplitude range, duration accuracy, boundary behavior)

---

## Phase 3: Transient Engine (Core DSP Loop)
- [x] Create DSP/TransientEngine.h — the master DSP coordinator
- [x] Implement `prepare(double sampleRate, int maxBlockSize)` — pre-allocate all buffers
- [x] Implement the core looping state machine:
  - State: PLAYING_TAIL — envelope is active, modulating audio
  - State: IN_SILENCE — gap between transients, output silence or pass-through at reduced level
  - State: TRIGGERING — transition sample, fire envelope.trigger() and enter PLAYING_TAIL
- [x] Implement `processBlock(juce::AudioBuffer<float>&, int numSamples)`:
  - For each sample: advance state machine, get envelope value, apply to audio
  - Per-sample processing for sample-accurate transient timing
- [x] Implement Input Mode routing:
  - External Audio: process the incoming buffer directly
  - White Noise: generate via `juce::Random` (pre-seeded, deterministic for reproducibility)
  - Pink Noise: use Voss-McCartney or Paul Kellet's algorithm
  - Sine Oscillator: simple phase-accumulator sine at configurable frequency
- [x] Implement dry/wet mix: `output = dry * (1 - mix) + wet * mix`
- [x] Ensure processBlock handles mono AND stereo (and arbitrary channel counts gracefully)
- [ ] Test: continuous looping produces transients at correct intervals (measure with sample counter)
- [ ] Test: no audio glitches at buffer boundaries
- [ ] Test: works at 44.1k, 48k, 88.2k, 96k, and 192k sample rates

---

## Phase 4: Doppler Processor
- [ ] Create DSP/DopplerProcessor.h — variable delay line for pitch-shift effect
- [ ] Implement circular buffer with fractional-sample interpolation (cubic or linear)
- [ ] Implement time-varying delay that increases over the tail duration:
  - At trigger: delay = minimum (near zero)
  - As tail progresses: delay smoothly increases, causing pitch to drop
  - Delay change rate derived from `PitchShift` parameter (semitones to speed ratio)
- [ ] The delay rate of change follows: `delayGrowthRate = 1.0 - pow(2.0, -pitchShiftSemitones / 12.0)`
- [ ] Implement anti-aliased interpolation (at minimum linear; prefer Hermite/cubic)
- [ ] Integrate DopplerProcessor into TransientEngine — activated only when shape == Doppler
- [ ] Test: verify pitch drops by expected semitone amount over tail duration
- [ ] Test: no aliasing artifacts or clicks during pitch sweep

---

## Phase 5: Host Tempo Sync
- [ ] Read BPM and time signature from `juce::AudioPlayHead::getCurrentPosition()`
- [ ] Calculate cycle duration from note value:
  - 1/4 note at 120 BPM = 500ms, etc.
  - Triplet values = note_duration * 2/3
- [ ] When Sync is ON: override Tail Length + Silence Gap with tempo-derived total cycle time
  - Tail Length stays user-defined, Silence Gap adjusts to fill remainder of the beat division
- [ ] Handle tempo changes mid-playback smoothly (no abrupt jumps)
- [ ] Handle missing tempo info gracefully (fall back to free-running mode)
- [ ] Test: at 120 BPM with 1/4 sync, verify exactly 2 transients per second

---

## Phase 6: Basic UI
- [ ] Design UI layout (target: ~500x350 px default size)
- [ ] Create TransientLookAndFeel with custom slider, button, and combobox styling
- [ ] Implement MainPanel as the top-level editor component
- [ ] Implement TransientControls panel:
  - [ ] Tail Length knob with ms readout
  - [ ] Silence Gap knob with ms readout
  - [ ] Transient Shape dropdown selector
  - [ ] Intensity knob with % readout
  - [ ] Pitch Shift knob (visible/enabled only when Doppler shape selected)
  - [ ] Mix knob with % readout
  - [ ] Sync toggle button
  - [ ] Note Value selector (visible/enabled only when Sync is ON)
  - [ ] Input Mode selector
- [ ] All controls attached to APVTS via SliderAttachment / ComboBoxAttachment / ButtonAttachment
- [ ] Implement EnvelopeVisualizer:
  - [ ] Draws the current envelope shape in real-time
  - [ ] Shows tail region, silence region, and loop point
  - [ ] Updates at ~30fps using juce::Timer or async repaint
  - [ ] Receives envelope shape data from processor via lock-free FIFO or atomic snapshot
- [ ] Make editor resizable with aspect ratio constraint
- [ ] Test: all controls respond correctly and update DSP in real-time
- [ ] Test: UI renders correctly on Retina/HiDPI displays

---

## Phase 7: Polish & Edge Cases
- [ ] Implement proper bypass behavior (when plugin is bypassed, pass audio through cleanly)
- [ ] Handle edge cases: zero tail length, zero silence gap, 100% intensity at boundaries
- [ ] Latency reporting: if any processing adds latency, report via `setLatencySamples()`
- [ ] Ensure thread safety: no data races between audio thread and UI thread
- [ ] Add parameter value tooltips
- [ ] Add input/output level meters (optional but nice)
- [ ] Profile CPU usage — target < 5% single-core at 48kHz stereo
- [ ] Memory audit: no leaks, no unnecessary allocations after prepareToPlay()

---

## Phase 8: Testing & Validation
- [ ] Unit tests for all envelope shapes (EnvelopeGeneratorTests.cpp)
- [ ] Unit tests for TransientEngine state machine (TransientEngineTests.cpp)
- [ ] Integration test: load plugin in AudioPluginHost, verify signal flow
- [ ] DAW compatibility testing: test in at least 2 major DAWs (Reaper, Ableton, Logic, etc.)
- [ ] Automation test: automate parameters and verify smooth transitions
- [ ] Preset save/load test: create preset, reload plugin, verify state matches
- [ ] Stress test: extreme parameter values, rapid parameter changes, high sample rates
- [ ] Cross-platform build verification (at least 2 of: macOS, Windows, Linux)

---

## Phase 9: Documentation & Release Prep
- [ ] Write Docs/DSP_DESIGN.md — full technical explanation of all DSP algorithms
- [ ] Update README.md with final build instructions and screenshots
- [ ] Create at least 5 factory presets demonstrating different transient characters
- [ ] Write user-facing documentation (what each parameter does, suggested use cases)
- [ ] Finalize license choice and add LICENSE file
- [ ] Create GitHub release workflow (optional: CI/CD with GitHub Actions)
- [ ] Tag v1.0.0

---

## Backlog / Future Ideas
- [ ] MIDI trigger mode: fire transients on MIDI note input instead of looping
- [ ] Sidechain input: use a separate audio source to trigger transients
- [ ] Modulation system: LFO or random modulation of tail length, gap, shape
- [ ] Preset browser with categories
- [ ] Waveform display of output signal
- [ ] A/B comparison mode
- [ ] Multiband transient creation (split into frequency bands, different shapes per band)
- [ ] Convolution mode: use short impulse responses as transient shapes
- [ ] CLAP format support
