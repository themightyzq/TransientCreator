#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline constexpr const char* TAIL_LENGTH       = "tailLength";
    inline constexpr const char* SILENCE_GAP        = "silenceGap";
    inline constexpr const char* SHAPE              = "transientShape";
    inline constexpr const char* INTENSITY          = "intensity";
    inline constexpr const char* PITCH_SHIFT        = "pitchShift";
    inline constexpr const char* MIX                = "mix";
    inline constexpr const char* SYNC_ENABLED       = "syncEnabled";
    inline constexpr const char* SYNC_NOTE          = "syncNote";
    inline constexpr const char* INPUT_MODE         = "inputMode";
    inline constexpr const char* OUTPUT_GAIN        = "outputGain";
    inline constexpr const char* LIMITER_ON         = "limiterOn";
    // Phase 4 new parameters
    inline constexpr const char* ATTACK_TIME        = "attackTime";
    inline constexpr const char* TRANSIENT_GAIN     = "transientGain";
    inline constexpr const char* ENVELOPE_TENSION   = "envelopeTension";
    inline constexpr const char* FILTER_HPF_FREQ    = "filterHPF";
    inline constexpr const char* FILTER_LPF_FREQ    = "filterLPF";
    inline constexpr const char* SINE_FREQ          = "sineFrequency";
    inline constexpr const char* DOPPLER_DIRECTION  = "dopplerDirection";
    inline constexpr const char* PRE_DELAY          = "preDelay";
    inline constexpr const char* HUMANIZE           = "humanize";
} // namespace ParamIDs

namespace ParamNames
{
    inline constexpr const char* TAIL_LENGTH       = "Tail Length";
    inline constexpr const char* SILENCE_GAP        = "Silence Gap";
    inline constexpr const char* SHAPE              = "Transient Shape";
    inline constexpr const char* INTENSITY          = "Intensity";
    inline constexpr const char* PITCH_SHIFT        = "Pitch Shift";
    inline constexpr const char* MIX                = "Mix";
    inline constexpr const char* SYNC_ENABLED       = "Sync to Host";
    inline constexpr const char* SYNC_NOTE          = "Sync Note Value";
    inline constexpr const char* INPUT_MODE         = "Input Mode";
    inline constexpr const char* OUTPUT_GAIN        = "Output Gain";
    inline constexpr const char* LIMITER_ON         = "Limiter";
    // Phase 4 new parameters
    inline constexpr const char* ATTACK_TIME        = "Attack Time";
    inline constexpr const char* TRANSIENT_GAIN     = "Transient Boost";
    inline constexpr const char* ENVELOPE_TENSION   = "Curve";
    inline constexpr const char* FILTER_HPF_FREQ    = "HPF";
    inline constexpr const char* FILTER_LPF_FREQ    = "LPF";
    inline constexpr const char* SINE_FREQ          = "Osc Frequency";
    inline constexpr const char* DOPPLER_DIRECTION  = "Doppler Direction";
    inline constexpr const char* PRE_DELAY          = "Pre-Delay";
    inline constexpr const char* HUMANIZE           = "Humanize";
} // namespace ParamNames

namespace ParamDefaults
{
    // Tail Length (ms)
    inline constexpr float TAIL_LENGTH_MIN     = 5.0f;
    inline constexpr float TAIL_LENGTH_MAX     = 5000.0f;
    inline constexpr float TAIL_LENGTH_DEFAULT = 50.0f;
    inline constexpr float TAIL_LENGTH_SKEW    = 0.4f;

    // Silence Gap (ms)
    inline constexpr float SILENCE_GAP_MIN     = 0.0f;
    inline constexpr float SILENCE_GAP_MAX     = 2000.0f;
    inline constexpr float SILENCE_GAP_DEFAULT = 100.0f;

    // Intensity (%)
    inline constexpr float INTENSITY_MIN       = 0.0f;
    inline constexpr float INTENSITY_MAX       = 100.0f;
    inline constexpr float INTENSITY_DEFAULT   = 75.0f;

    // Pitch Shift (semitones)
    inline constexpr float PITCH_SHIFT_MIN     = 0.0f;
    inline constexpr float PITCH_SHIFT_MAX     = 24.0f;
    inline constexpr float PITCH_SHIFT_DEFAULT = 12.0f;

    // Mix (Dry/Wet %)
    inline constexpr float MIX_MIN             = 0.0f;
    inline constexpr float MIX_MAX             = 100.0f;
    inline constexpr float MIX_DEFAULT         = 100.0f;

    // Sync to Host
    inline constexpr bool SYNC_ENABLED_DEFAULT = false;

    // Transient Shape
    inline constexpr int SHAPE_DEFAULT         = 0;

    // Sync Note Value
    inline constexpr int SYNC_NOTE_DEFAULT     = 2;

    // Input Mode
    inline constexpr int INPUT_MODE_DEFAULT    = 0;

    // Output Gain (dB)
    inline constexpr float OUTPUT_GAIN_MIN     = -24.0f;
    inline constexpr float OUTPUT_GAIN_MAX     = 24.0f;
    inline constexpr float OUTPUT_GAIN_DEFAULT = 0.0f;

    // Limiter
    inline constexpr bool LIMITER_ON_DEFAULT   = true;

    // Attack Time (ms) — 4A
    inline constexpr float ATTACK_TIME_MIN     = 0.0f;
    inline constexpr float ATTACK_TIME_MAX     = 10.0f;
    inline constexpr float ATTACK_TIME_DEFAULT = 0.1f;
    inline constexpr float ATTACK_TIME_SKEW    = 0.5f;

    // Transient Boost (dB) — 4B
    inline constexpr float TRANSIENT_GAIN_MIN     = 0.0f;
    inline constexpr float TRANSIENT_GAIN_MAX     = 24.0f;
    inline constexpr float TRANSIENT_GAIN_DEFAULT = 0.0f;

    // Envelope Tension / Curve — 4C
    inline constexpr float ENVELOPE_TENSION_MIN     = 0.1f;
    inline constexpr float ENVELOPE_TENSION_MAX     = 5.0f;
    inline constexpr float ENVELOPE_TENSION_DEFAULT = 1.0f;

    // HPF Frequency (Hz) — 4D
    inline constexpr float HPF_FREQ_MIN     = 20.0f;
    inline constexpr float HPF_FREQ_MAX     = 2000.0f;
    inline constexpr float HPF_FREQ_DEFAULT = 20.0f;
    inline constexpr float HPF_FREQ_SKEW    = 0.3f;

    // LPF Frequency (Hz) — 4D
    inline constexpr float LPF_FREQ_MIN     = 200.0f;
    inline constexpr float LPF_FREQ_MAX     = 20000.0f;
    inline constexpr float LPF_FREQ_DEFAULT = 20000.0f;
    inline constexpr float LPF_FREQ_SKEW    = 0.3f;

    // Sine Oscillator Frequency (Hz) — 4E
    inline constexpr float SINE_FREQ_MIN     = 20.0f;
    inline constexpr float SINE_FREQ_MAX     = 8000.0f;
    inline constexpr float SINE_FREQ_DEFAULT = 440.0f;
    inline constexpr float SINE_FREQ_SKEW    = 0.3f;

    // Doppler Direction — 4F
    inline constexpr int DOPPLER_DIRECTION_DEFAULT = 0;

    // Pre-Delay (ms) — 4G
    inline constexpr float PRE_DELAY_MIN     = 0.0f;
    inline constexpr float PRE_DELAY_MAX     = 50.0f;
    inline constexpr float PRE_DELAY_DEFAULT = 0.0f;

    // Humanize (%) — 4H
    inline constexpr float HUMANIZE_MIN     = 0.0f;
    inline constexpr float HUMANIZE_MAX     = 100.0f;
    inline constexpr float HUMANIZE_DEFAULT = 0.0f;

    // Parameter smoothing ramp time (seconds)
    inline constexpr double SMOOTHING_RAMP_SEC = 0.02;
} // namespace ParamDefaults

// Transient shape choice strings (must match EnvelopeShape enum order)
inline const juce::StringArray shapeChoices
{
    "Exponential", "Linear", "Logarithmic", "Doppler",
    "Reverse Sawtooth", "Gaussian", "Double Tap", "Percussive"
};

// Sync note value choice strings
inline const juce::StringArray syncNoteChoices
{
    "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
    "1/4T", "1/8T", "1/16T"
};

// Input mode choice strings
inline const juce::StringArray inputModeChoices
{
    "External Audio", "White Noise", "Pink Noise", "Sine Oscillator"
};

// Doppler direction choice strings — 4F
inline const juce::StringArray dopplerDirectionChoices
{
    "Recede", "Approach", "Fly-by"
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
