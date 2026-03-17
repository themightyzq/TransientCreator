# Transient Creator

**A JUCE-based VST3 plugin that generates punchy transient events from sustained audio using Doppler-inspired envelope shaping.**

---

## What Is This?

Transient Creator is NOT a transient shaper — it's a transient CREATOR. While transient shapers modify existing attacks and sustains, Transient Creator takes any audio input (or its own internal source) and rhythmically sculpts it into short, punchy transient impulses.

Inspired by the Doppler effect: each transient starts at its peak energy (the "apex" — as if the sound source is at its closest point) and then decays outward, compressing sustained audio into tight, rhythmic impacts.

### Key Features
- **Multiple transient shapes**: Exponential, Linear, Logarithmic, Doppler, Gaussian, Double-Tap, Percussive
- **Looping engine**: Continuously fires transients with user-defined tail length and silence gaps
- **Host sync**: Lock transient timing to your DAW's tempo with note-value subdivisions
- **Real-time visualization**: See your transient envelope shape in real-time
- **Dual input modes**: Process incoming audio or use the internal oscillator/noise source

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
After building, the VST3 plugin will be in:
- **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
- **Windows**: `C:\Program Files\Common Files\VST3\`
- **Linux**: `~/.vst3/`

---

## Project Structure
```
Source/
├── PluginProcessor.*       — Audio processing entry point
├── PluginEditor.*          — GUI entry point
├── DSP/                    — All signal processing code
│   ├── TransientEngine     — Core looping transient generator
│   ├── EnvelopeGenerator   — Envelope shape definitions & computation
│   └── DopplerProcessor    — Variable delay line for pitch-shift decay
├── Parameters/             — APVTS parameter layout & IDs
└── UI/                     — All visual components
    ├── LookAndFeel/        — Custom styling
    ├── Components/         — Reusable UI widgets
    └── Panels/             — Layout panels
```

---

## Dependencies
- [JUCE](https://juce.com/) — Cross-platform C++ audio framework
- CMake — Build system

---

## Documentation
- `claude.md` — AI agent operations manual
- `TODO.md` — Development roadmap & task tracking
- `Docs/DSP_DESIGN.md` — Technical DSP architecture documentation

---

## License
[TBD — Choose your license]

---

## Contributing
See `claude.md` for code standards and `TODO.md` for current development priorities.
