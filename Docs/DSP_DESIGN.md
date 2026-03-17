# DSP Design Document — Transient Creator

> Living document. Updated as implementation progresses.

---

## 1. Core Concept: Apex-Decay Transient Model

Transient Creator generates short, punchy transient events by applying shaped amplitude envelopes to audio. The conceptual model is inspired by the Doppler effect: imagine a sound source at its closest point of approach (the **apex**) that then rapidly recedes. At the apex, energy is maximum; as the source recedes, amplitude drops, pitch falls, and the sound compresses into a sharp impulse followed by a shaped tail.

**Key insight:** Unlike a transient *shaper* (which modifies existing transients), Transient Creator *generates* transients by imposing envelope shapes onto sustained or arbitrary audio. The plugin loops continuously, firing a new transient at regular intervals.

### Signal Flow

```
Input Audio (or Internal Source)
        │
        ▼
┌─────────────────┐
│  Input Router    │  ← Selects: External, White Noise, Pink Noise, Sine
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Envelope Gen    │  ← Generates amplitude envelope per shape type
│  × Audio Signal  │  ← Multiplies envelope by audio sample-by-sample
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Doppler Proc    │  ← (Doppler mode only) Variable delay for pitch shift
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Dry/Wet Mix     │  ← Blends processed with original
└────────┬────────┘
         │
         ▼
      Output
```

---

## 2. Looping State Machine

The TransientEngine operates as a three-state machine that loops indefinitely:

```
    ┌──────────────────────────────────────┐
    │                                      │
    ▼                                      │
TRIGGERING ──► PLAYING_TAIL ──► IN_SILENCE ┘
 (1 sample)     (tail_ms)       (gap_ms)
```

- **TRIGGERING**: Fires `envelope.trigger()`, transitions to PLAYING_TAIL on the same sample.
- **PLAYING_TAIL**: Envelope is active. Each audio sample is multiplied by `envelope.getNextSample()`. Duration = `tailLength` in samples (derived from ms and sample rate).
- **IN_SILENCE**: Output is silence (or heavily attenuated). Duration = `silenceGap` in samples. After this period, transitions back to TRIGGERING.

### Timing Calculation

```
tailSamples = (tailLengthMs / 1000.0) * sampleRate
gapSamples  = (silenceGapMs / 1000.0) * sampleRate
cycleSamples = tailSamples + gapSamples
```

When host sync is active, `cycleSamples` is derived from BPM and note division, and `gapSamples = cycleSamples - tailSamples`.

---

## 3. Envelope Shapes — Mathematics

All envelopes output amplitude values in range [0.0, 1.0]. `t` represents elapsed samples since trigger. `T` represents total tail duration in samples.

### 3.1 Exponential Decay (Default)

The most "punchy" shape. Sharp apex, rapid falloff.

```
amplitude(t) = exp(-t * decayRate)
decayRate = -ln(threshold) / T
```

Where `threshold` is the minimum amplitude at the end of the tail (e.g., 0.001 = -60dB). This ensures the envelope reaches near-zero at exactly `T` samples.

### 3.2 Linear Decay

Straight-line drop from 1.0 to 0.0.

```
amplitude(t) = 1.0 - (t / T)
```

### 3.3 Logarithmic Decay

Slower initial drop that accelerates toward the end. Produces a softer, rounder transient.

```
amplitude(t) = 1.0 - log(1 + t * k) / log(1 + T * k)
```

Where `k` controls the curvature (larger k = more logarithmic character). Default `k = 10.0`.

### 3.4 Doppler Curve

Amplitude decay combined with pitch-shift downward via variable delay line. The amplitude envelope itself follows exponential decay; the pitch shift is handled by the DopplerProcessor.

```
amplitude(t) = exp(-t * decayRate)   // Same as exponential
pitch_ratio(t) = 1.0 - (delayGrowthRate * t / T)  // Decreasing pitch
```

### 3.5 Reverse Sawtooth

Instant maximum, linear fall, abrupt cutoff.

```
amplitude(t) = 1.0 - (t / T)    for t < T
amplitude(t) = 0.0              for t >= T
```

Identical to Linear Decay in shape, but the key difference is the *hard cutoff* at the boundary — no crossfade smoothing at the end (though crossfade is still applied at the re-trigger point).

### 3.6 Gaussian Pulse

Bell-curve shaped, symmetric micro-transient centered at the start of the tail.

```
amplitude(t) = exp(-0.5 * (t / sigma)^2)
sigma = T / 4.0    // Places ~95% of energy in first half of tail
```

### 3.7 Double Tap

Two rapid exponential hits with configurable inter-tap spacing.

```
tap1(t) = exp(-t * decayRate1)                     for t < T/2
tap2(t) = exp(-(t - tapSpacing) * decayRate2)      for t >= tapSpacing
amplitude(t) = max(tap1(t), tap2(t))
```

Default `tapSpacing = T * 0.3` (second tap fires at 30% of tail duration).

### 3.8 Percussive

Models a struck membrane: fast attack (~1ms), short resonant body, exponential decay.

```
attackSamples = 0.001 * sampleRate    // 1ms attack
bodySamples = T * 0.15               // 15% of tail is "body"

amplitude(t) = t / attackSamples                           for t < attackSamples (attack)
amplitude(t) = 1.0 - 0.3 * ((t - attackSamples) / bodySamples)  for attack <= t < bodySamples (body)
amplitude(t) = 0.7 * exp(-(t - bodySamples) * decayRate)   for t >= bodySamples (decay)
```

---

## 4. Doppler Pitch-Shift — Variable Delay Line

The DopplerProcessor implements pitch shifting through a time-varying delay line. As the delay increases over time, the read pointer falls behind the write pointer, effectively stretching the waveform and lowering pitch.

### Theory

A constant delay produces no pitch change. A *linearly increasing* delay produces a constant pitch shift downward. The rate of delay increase determines the pitch ratio:

```
pitchRatio = 1.0 - d'(t)
```

Where `d'(t)` is the rate of change of delay in samples per sample.

For a target pitch shift of `S` semitones downward:

```
targetRatio = pow(2.0, -S / 12.0)
delayGrowthRate = 1.0 - targetRatio
```

### Implementation

- **Circular buffer**: Pre-allocated to accommodate maximum delay (derived from maximum tail length and maximum pitch shift).
- **Write pointer**: Advances by 1 sample per sample (standard).
- **Read pointer**: `readPos = writePos - currentDelay`, where `currentDelay` increases from ~0 to `maxDelay` over the tail duration.
- **Interpolation**: Fractional sample positions require interpolation. Cubic Hermite interpolation provides good quality with low computation.

### Maximum Delay Calculation

```
maxDelaySamples = tailSamples * delayGrowthRate
bufferSize = nextPowerOf2(maxDelaySamples + guardSamples)
```

Where `guardSamples` accounts for interpolation lookahead (4 samples for cubic).

---

## 5. Anti-Aliasing at Trigger Boundaries

When the state machine transitions from IN_SILENCE back to TRIGGERING, there is a risk of amplitude discontinuity (the previous sample was 0.0 or near-zero, and the new envelope starts at 1.0). This produces an audible click.

### Strategy: Short Crossfade

At each re-trigger boundary, apply a crossfade of `N` samples (default N=32):

```
for i in 0..N:
    fadeFactor = i / N                    // 0.0 → 1.0
    output[i] = previous[i] * (1 - fadeFactor) + triggered[i] * fadeFactor
```

This requires buffering the last `N` samples of the silence gap to blend with the first `N` samples of the new transient.

### Alternative: Envelope starts from zero

For shapes that naturally start from zero (like Percussive with its attack phase), the crossfade is unnecessary. The EnvelopeGenerator tracks whether the current shape has a zero-start and skips crossfading when safe.

---

## 6. Sample Rate Independence

All timing calculations use sample counts derived from milliseconds and the current sample rate. When `prepareToPlay()` is called:

1. Recalculate `tailSamples = (tailLengthMs / 1000.0) * sampleRate`
2. Recalculate `gapSamples = (silenceGapMs / 1000.0) * sampleRate`
3. Recalculate all envelope decay rates (which depend on `tailSamples`)
4. Resize/reallocate the Doppler delay buffer if needed
5. Reset SmoothedValue ramp times

This ensures identical behavior at 44.1kHz, 48kHz, 88.2kHz, 96kHz, and 192kHz.

---

## 7. Host Tempo Sync

When sync is enabled, the total cycle time (tail + gap) is derived from the host's BPM and the selected note value:

```
beatDuration_ms = 60000.0 / bpm
noteDuration_ms = beatDuration_ms * noteMultiplier

noteMultiplier values:
  1/1  = 4.0
  1/2  = 2.0
  1/4  = 1.0
  1/8  = 0.5
  1/16 = 0.25
  1/32 = 0.125
  1/4T = 2.0/3.0
  1/8T = 1.0/3.0
  1/16T = 1.0/6.0
```

The user's Tail Length setting is preserved. The Silence Gap is automatically adjusted:

```
silenceGap_ms = max(0, noteDuration_ms - tailLength_ms)
```

If the tail length exceeds the note duration, the tail is truncated to fit.

Tempo changes are handled by recalculating on each `processBlock()` call using the latest `AudioPlayHead` position. SmoothedValue prevents abrupt timing jumps.
