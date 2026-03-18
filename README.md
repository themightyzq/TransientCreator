# Transient Creator

**A JUCE-based VST3 plugin that generates punchy transient events from sustained audio using Doppler-inspired envelope shaping.**

---

## What Is This?

Transient Creator is NOT a transient shaper — it's a transient CREATOR. While transient shapers modify existing attacks and sustains, Transient Creator takes any audio input (or its own internal source) and rhythmically sculpts it into short, punchy transient impulses.

Inspired by the Doppler effect: each transient starts at its peak energy (the "apex" — as if the sound source is at its closest point) and then decays outward, compressing sustained audio into tight, rhythmic impacts.

### Key Features
- **8 transient shapes**: Exponential, Linear, Logarithmic, Doppler, Reverse Sawtooth, Gaussian, Double-Tap, Percussive
- **Attack control**: Shape the onset from instant stick hit (0ms) to soft mallet (10ms)
- **Transient boost**: Amplify peaks up to +24dB for aggressive transients
- **Envelope tension/curve**: Warp any shape from gentle to extremely snappy
- **Doppler pitch-shift**: Variable delay with Recede, Approach, and Fly-by modes
- **Output filters**: HPF (20–2000Hz) and LPF (200–20kHz) for frequency shaping
- **Pre-delay**: Offset transients up to 50ms for groove shaping
- **Humanize**: Per-cycle random variation (±20%) for organic feel
- **Looping engine**: Continuously fires transients with user-defined timing
- **Host sync**: Lock transient timing to DAW tempo with note-value subdivisions
- **4 input sources**: External audio, White Noise, Pink Noise, Sine Oscillator (tunable 20–8000Hz)
- **Output gain + brickwall limiter**: Push transients hard without clipping
- **Real-time envelope visualizer**: See your transient shape with attack and tension in real-time

---

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Tail Length | 5–5000 ms | 50 ms | Duration of the transient decay |
| Silence Gap | 0–2000 ms | 100 ms | Silence between transients (overridden by Sync) |
| Shape | 8 shapes | Exponential | Envelope curve type |
| Attack Time | 0–10 ms | 0.1 ms | Onset ramp time (0 = instant apex) |
| Intensity | 0–100% | 75% | How much the envelope reshapes audio |
| Transient Boost | 0–24 dB | 0 dB | Amplify peak relative to sustained level |
| Curve (Tension) | 0.1–5.0 | 1.0 | Warp envelope shape (< 1 gentle, > 1 snappy) |
| Pitch Shift | 0–24 st | 12 st | Doppler pitch drop amount (Doppler mode only) |
| Doppler Direction | 3 modes | Recede | Recede, Approach, or Fly-by sweep |
| Mix | 0–100% | 100% | Dry/wet blend |
| HPF | 20–2000 Hz | 20 Hz | High-pass filter on wet signal |
| LPF | 200–20000 Hz | 20000 Hz | Low-pass filter on wet signal |
| Pre-Delay | 0–50 ms | 0 ms | Offset transient timing for groove shaping |
| Humanize | 0–100% | 0% | Per-cycle random variation on timing |
| Output Gain | -24–+24 dB | 0 dB | Output level boost/cut |
| Limiter | ON/OFF | ON | Brickwall limiter at -0.3dBFS |
| Sync | ON/OFF | OFF | Lock timing to DAW tempo |
| Sync Note | 9 values | 1/4 | Beat subdivision when synced |
| Input Mode | 4 sources | External | Audio source selection |
| Osc Frequency | 20–8000 Hz | 440 Hz | Sine oscillator pitch (Sine mode only) |

---

## Building

### Prerequisites
- **CMake** 3.22 or higher
- **C++ compiler** with C++17 support (GCC 9+, Clang 10+, MSVC 2019+)
- **JUCE** 7.x (included as git submodule)
- **Platform SDKs**: Xcode CLI tools (macOS), Visual Studio (Windows), ALSA/JACK dev libs (Linux)

### Clone & Build
```bash
git clone --recursive https://github.com/YOUR_USERNAME/TransientCreator.git
cd TransientCreator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Plugin Installation
After building, the VST3 plugin will be auto-installed to:
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
- **Windows**: `C:\Program Files\Common Files\VST3\`
- **Linux**: `~/.vst3/`

---

## Project Structure
```
Source/
├── PluginProcessor.*          — Audio processing entry point
├── PluginEditor.*             — GUI entry point
├── DSP/
│   ├── EnvelopeConstants.h    — Shared envelope math constants
│   ├── EnvelopeGenerator.*    — 8 envelope shapes with attack, tension, humanize
│   ├── TransientEngine.*      — Core DSP loop, filters, pre-delay, mixing
│   └── DopplerProcessor.*     — Variable delay line with 3 direction modes
├── Parameters/
│   └── ParameterLayout.*      — All 20 parameter definitions (IDs, names, ranges)
└── UI/
    ├── LookAndFeel/           — Dark theme styling
    ├── Components/            — Envelope visualizer, parameter controls
    └── Panels/                — Main layout panel
```

---

## Signal Chain
```
Input (External/Noise/Sine) → Envelope × (Intensity + Boost) → Doppler
  → Pre-Delay → HPF → LPF → Dry/Wet Mix → Output Gain → Limiter → DAW
```

---

## Dependencies
- [JUCE](https://juce.com/) — Cross-platform C++ audio framework (modules: `juce_audio_utils`, `juce_dsp`)
- CMake — Build system

---

## Documentation
- `claude.md` — AI agent operations manual & architecture rules
- `TODO.md` — Development roadmap & task tracking
- `Docs/DSP_DESIGN.md` — Technical DSP architecture documentation
- `Docs/PRESETS.md` — Factory preset definitions

---

## License
[TBD — Choose your license]

---

## Contributing
See `claude.md` for code standards and `TODO.md` for current development priorities.
