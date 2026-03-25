#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline constexpr const char* TAIL_LENGTH       = "tailLength";
    inline constexpr const char* SILENCE_GAP        = "silenceGap";
    inline constexpr const char* SHAPE              = "transientShape";
    inline constexpr const char* MIX                = "mix";
    inline constexpr const char* SYNC_ENABLED       = "syncEnabled";
    inline constexpr const char* SYNC_NOTE          = "syncNote";
    inline constexpr const char* INPUT_MODE         = "inputMode";
    inline constexpr const char* OUTPUT_GAIN        = "outputGain";
    inline constexpr const char* LIMITER_ON         = "limiterOn";
    // Phase 4 new parameters
    inline constexpr const char* ATTACK_TIME        = "attackTime";
    inline constexpr const char* TRANSIENT_GAIN     = "transientGain";
    inline constexpr const char* PITCH_START        = "pitchStart";
    inline constexpr const char* PITCH_END          = "pitchEnd";
    inline constexpr const char* SINE_FREQ          = "sineFrequency";
    inline constexpr const char* HUMANIZE           = "humanize";
    inline constexpr const char* SUSTAIN_HOLD       = "sustainHold";
} // namespace ParamIDs

namespace ParamNames
{
    inline constexpr const char* TAIL_LENGTH       = "Tail Length";
    inline constexpr const char* SILENCE_GAP        = "Silence Gap";
    inline constexpr const char* SHAPE              = "Transient Shape";
    inline constexpr const char* MIX                = "Mix";
    inline constexpr const char* SYNC_ENABLED       = "Sync to Host";
    inline constexpr const char* SYNC_NOTE          = "Sync Note Value";
    inline constexpr const char* INPUT_MODE         = "Input Mode";
    inline constexpr const char* OUTPUT_GAIN        = "Output Gain";
    inline constexpr const char* LIMITER_ON         = "Limiter";
    // Phase 4 new parameters
    inline constexpr const char* ATTACK_TIME        = "Attack Time";
    inline constexpr const char* TRANSIENT_GAIN     = "Transient Boost";
    inline constexpr const char* PITCH_START        = "Pitch Start";
    inline constexpr const char* PITCH_END          = "Pitch End";
    inline constexpr const char* SINE_FREQ          = "Osc Frequency";
    inline constexpr const char* HUMANIZE           = "Humanize";
    inline constexpr const char* SUSTAIN_HOLD       = "Hold";
} // namespace ParamNames

namespace ParamDefaults
{
    // Tail Length (ms)
    inline constexpr float TAIL_LENGTH_MIN     = 5.0f;
    inline constexpr float TAIL_LENGTH_MAX     = 5000.0f;
    inline constexpr float TAIL_LENGTH_DEFAULT = 150.0f;
    inline constexpr float TAIL_LENGTH_SKEW    = 0.4f;

    // Silence Gap (ms)
    inline constexpr float SILENCE_GAP_MIN     = 0.0f;
    inline constexpr float SILENCE_GAP_MAX     = 2000.0f;
    inline constexpr float SILENCE_GAP_DEFAULT = 100.0f;

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
    inline constexpr int INPUT_MODE_SINE_INDEX = 3;  // Index of Sine Oscillator in inputModeChoices

    // Output Gain (dB)
    inline constexpr float OUTPUT_GAIN_MIN     = -24.0f;
    inline constexpr float OUTPUT_GAIN_MAX     = 24.0f;
    inline constexpr float OUTPUT_GAIN_DEFAULT = 0.0f;

    // Limiter
    inline constexpr bool LIMITER_ON_DEFAULT   = true;

    // Attack Time (ms) — 4A
    inline constexpr float ATTACK_TIME_MIN     = 0.0f;
    inline constexpr float ATTACK_TIME_MAX     = 500.0f;
    inline constexpr float ATTACK_TIME_DEFAULT = 0.1f;
    inline constexpr float ATTACK_TIME_SKEW    = 0.3f;

    // Transient Boost (dB) — 4B
    inline constexpr float TRANSIENT_GAIN_MIN     = 0.0f;
    inline constexpr float TRANSIENT_GAIN_MAX     = 24.0f;
    inline constexpr float TRANSIENT_GAIN_DEFAULT = 0.0f;

    // Pitch Start (semitones)
    inline constexpr float PITCH_START_MIN     = -24.0f;
    inline constexpr float PITCH_START_MAX     = 24.0f;
    inline constexpr float PITCH_START_DEFAULT = 0.0f;

    // Pitch End (semitones)
    inline constexpr float PITCH_END_MIN     = -24.0f;
    inline constexpr float PITCH_END_MAX     = 24.0f;
    inline constexpr float PITCH_END_DEFAULT = 0.0f;

    // Sine Oscillator Frequency (Hz) — 4E
    inline constexpr float SINE_FREQ_MIN     = 20.0f;
    inline constexpr float SINE_FREQ_MAX     = 8000.0f;
    inline constexpr float SINE_FREQ_DEFAULT = 440.0f;
    inline constexpr float SINE_FREQ_SKEW    = 0.3f;

    // Humanize (%) — 4H
    inline constexpr float HUMANIZE_MIN     = 0.0f;
    inline constexpr float HUMANIZE_MAX     = 100.0f;
    inline constexpr float HUMANIZE_DEFAULT = 0.0f;

    // Sustain Hold (%) — 7A
    inline constexpr float SUSTAIN_HOLD_MIN     = 0.0f;
    inline constexpr float SUSTAIN_HOLD_MAX     = 50.0f;
    inline constexpr float SUSTAIN_HOLD_DEFAULT = 0.0f;

    // Parameter smoothing ramp time (seconds)
    inline constexpr double SMOOTHING_RAMP_SEC = 0.02;
} // namespace ParamDefaults

// Transient shape choice strings (must match EnvelopeShape enum order)
inline const juce::StringArray shapeChoices
{
    "Exponential", "Linear", "Logarithmic",
    "Reverse Sawtooth", "Double Tap", "Percussive"
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

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
