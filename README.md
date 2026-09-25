# Transient Creator

Transient Creator generates new, sharply shaped transient hits synced to a rhythm, from
incoming audio or its own noise and sine generators. It is a transient creator, not a
transient shaper: it builds new transient events with a looping envelope engine and a
fully interactive, hand-drawable curve editor, with a pitch sweep across each hit. For
sound designers and producers shaping drums and percussive material. VST3, AU, and
Standalone on macOS; VST3 and Standalone on Windows and Linux. Built with JUCE.

## Install

Download the latest build for your platform from the
[Releases page](https://github.com/themightyzq/TransientCreator/releases/latest):
`TransientCreator-macOS.zip` (VST3, AU and Standalone, universal),
`TransientCreator-Windows-VST3.zip`, `TransientCreator-Linux-VST3.zip`. The builds are
unsigned: on macOS, right-click and Open the Standalone app the first time.

Copy the `.vst3` (and, on macOS, the `.component` for AU hosts like Logic) to your
plugin folder:

| Platform | VST3 location | AU location |
|----------|---------------|-------------|
| macOS | `~/Library/Audio/Plug-Ins/VST3/` | `~/Library/Audio/Plug-Ins/Components/` |
| Windows | `C:\Program Files\Common Files\VST3\` | n/a |
| Linux | `~/.vst3/` | n/a |

Restart your DAW. The plugin appears as Transient Creator under Effects > Dynamics >
Tools.

Requires macOS 11.0 or later.

## Use

The preset bar in the header (top left) loads any of 5 factory presets or your own saved
presets. Save writes the current settings to
`~/Library/Audio/Presets/ZQ SFX/Transient Creator/` as a `.tcpreset` file, and the `...`
menu renames, deletes, or reveals that folder.

1. Insert Transient Creator on a track, or use Standalone mode.
2. Choose an input: External Audio processes your track; White Noise, Pink Noise, or
   Sine generate sound internally.
3. Set Tail Length to control how long each transient lasts.
4. Set Silence Gap to control the space between transients, or enable Sync for
   tempo-locked timing.
5. Pick a shape from the dropdown, or click directly on the curve to add or move
   breakpoints.
6. Adjust Pitch Start and Pitch End for a pitch sweep across the transient.
7. Dial in Boost to emphasize the transient peak.

### Curve editor controls

- Click in empty space to add a breakpoint.
- Drag a breakpoint to move it.
- Right-click a breakpoint to delete it.
- Alt+drag between breakpoints to bend the curve segment.
- Double-click to reset to the current preset shape.

## Build

Requirements: CMake 3.22+, a C++17 compiler, platform SDK (Xcode CLI, Visual Studio, or
ALSA+X11 dev libs).

```bash
git clone --recursive https://github.com/themightyzq/TransientCreator.git
cd TransientCreator
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

On macOS this produces a Universal Binary (arm64 + x86_64) with VST3 and AU, and copies
both into your user plug-in folders automatically after building.

## Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Tail Length | 5 ms to 5 s | 150 ms | Duration of the transient decay |
| Silence Gap | 0 to 2 s | 100 ms | Silence between transients |
| Shape | 6 presets | Exponential | Envelope curve starting point |
| Attack | 0 to 500 ms | 0.1 ms | Onset ramp time |
| Hold | 0 to 50% | 0% | Hold at peak before decay (% of tail) |
| Boost | 0 to 24 dB | 0 dB | Amplify the transient peak |
| Pitch Start | -24 to +24 st | 0 st | Pitch at transient start |
| Pitch End | -24 to +24 st | 0 st | Pitch at transient end |
| Mix | 0 to 100% | 100% | Dry/wet blend |
| Gain | -24 to +24 dB | 0 dB | Output level |
| Humanize | 0 to 100% | 0% | Random timing variation |
| Limiter | On/Off | On | Brickwall output limiter |
| Sync | On/Off | Off | Lock timing to DAW tempo |
| Sync Note | 1/1 to 1/16T | 1/4 | Beat subdivision |
| Input Mode | 4 sources | White Noise | Audio source |
| Osc Frequency | 20 to 8000 Hz | 440 Hz | Sine oscillator pitch |

## Licence

Copyright (c) 2026 ZQ SFX. GPL-3.0-or-later. See `LICENSE`. Built with JUCE, used
under its AGPLv3 option, which GPL-3.0 is compatible with. Releases before 2026-09-21
were offered under the MIT License; that grant stands for those copies.

ZQ SFX, https://www.zq-sfx.com, connect@zq-sfx.com.
