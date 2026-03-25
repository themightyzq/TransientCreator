# Transient Creator

A VST3/Standalone audio plugin for creating punchy transient events from any audio source. Built with JUCE.

---

## What It Does

Transient Creator takes any audio input (or its own internal generators) and rhythmically sculpts it into short, shaped transient impulses. It's a transient **creator**, not a transient shaper -- it generates new transient events using a looping envelope engine with a fully interactive curve editor.

### Use Cases
- **Sound design**: Create custom impacts, clicks, hits, and percussive textures
- **Game audio**: Design one-shot transients with precise pitch sweeps and timing
- **Film/post**: Build layered whooshes, risers, and punchy stingers
- **Music production**: Add rhythmic transient layers synced to DAW tempo

---

## Features

- **Interactive curve editor** -- click to add breakpoints, drag to reshape, Alt+drag to bend segments. 6 preset shapes as starting points.
- **6 envelope shapes**: Exponential, Linear, Logarithmic, Reverse Sawtooth, Double Tap, Percussive
- **Pitch Start / Pitch End** -- sweep pitch from -24 to +24 semitones across the transient. Pitch processes audio before the envelope so timing is always exact.
- **Attack control** (0-500ms) -- from instant stick hit to slow fade-in
- **Transient Boost** -- amplify peaks up to +24dB
- **Sustain Hold** -- hold at peak amplitude before decay begins
- **Host tempo sync** -- lock transient timing to DAW tempo with note-value subdivisions
- **4 input sources**: External audio, White Noise, Pink Noise, Sine Oscillator
- **Humanize** -- per-cycle random timing variation for organic feel
- **Output gain + brickwall limiter**
- **Real-time envelope visualizer** with playhead, breakpoint editor, and rate display

---

## Installation

### From Releases

Download the latest build for your platform from the [Releases](../../releases) page. Copy the `.vst3` file to your plugin folder:

| Platform | VST3 Location |
|----------|---------------|
| macOS | `~/Library/Audio/Plug-Ins/VST3/` |
| Windows | `C:\Program Files\Common Files\VST3\` |
| Linux | `~/.vst3/` |

Restart your DAW. The plugin appears as **Transient Creator** under Effects > Dynamics > Tools.

### Building from Source

**Requirements:** CMake 3.22+, C++17 compiler, platform SDK (Xcode CLI / Visual Studio / ALSA+X11 dev libs)

```bash
git clone --recursive https://github.com/themightyzq/TransientCreator.git
cd TransientCreator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On macOS, the build produces a Universal Binary (arm64 + x86_64). The VST3 is automatically copied to your plugin folder after building.

---

## Quick Start

1. **Insert** Transient Creator on a track (or use Standalone mode)
2. **Choose an input**: External Audio processes your track; White Noise/Pink Noise/Sine generate sound internally
3. **Set Tail Length** to control how long each transient lasts
4. **Set Gap** to control the silence between transients (or enable SYNC for tempo-locked timing)
5. **Pick a shape** from the dropdown, or click directly on the curve to add/move breakpoints
6. **Adjust Pitch Start/End** for pitch sweeps across the transient
7. **Dial in Boost** to emphasize the transient peak

### Curve Editor Controls
- **Click** in empty space to add a breakpoint
- **Drag** a breakpoint to move it
- **Right-click** a breakpoint to delete it
- **Alt+drag** between breakpoints to bend the curve segment
- **Double-click** to reset to the current preset shape

---

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Tail Length | 5 ms -- 5 s | 150 ms | Duration of the transient decay |
| Silence Gap | 0 -- 2 s | 100 ms | Silence between transients |
| Shape | 6 presets | Exponential | Envelope curve starting point |
| Attack | 0 -- 500 ms | 0.1 ms | Onset ramp time |
| Hold | 0 -- 50% | 0% | Hold at peak before decay (% of tail) |
| Boost | 0 -- 24 dB | 0 dB | Amplify the transient peak |
| Pitch Start | -24 -- +24 st | 0 st | Pitch at transient start |
| Pitch End | -24 -- +24 st | 0 st | Pitch at transient end |
| Mix | 0 -- 100% | 100% | Dry/wet blend |
| Gain | -24 -- +24 dB | 0 dB | Output level |
| Humanize | 0 -- 100% | 0% | Random timing variation |
| Limiter | On/Off | On | Brickwall output limiter |
| Sync | On/Off | Off | Lock timing to DAW tempo |
| Sync Note | 1/1 -- 1/16T | 1/4 | Beat subdivision |
| Input Mode | 4 sources | External | Audio source |
| Osc Frequency | 20 -- 8000 Hz | 440 Hz | Sine oscillator pitch |

---

## License

MIT License. See [LICENSE](LICENSE) for details.

---

## Credits

Built by [ZQSFX](https://github.com/themightyzq) using [JUCE](https://juce.com/).
