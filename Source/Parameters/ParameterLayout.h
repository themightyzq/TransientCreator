#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline constexpr const char* TAIL_LENGTH    = "tailLength";
    inline constexpr const char* SILENCE_GAP    = "silenceGap";
    inline constexpr const char* SHAPE          = "transientShape";
    inline constexpr const char* INTENSITY      = "intensity";
    inline constexpr const char* PITCH_SHIFT    = "pitchShift";
    inline constexpr const char* MIX            = "mix";
    inline constexpr const char* SYNC_ENABLED   = "syncEnabled";
    inline constexpr const char* SYNC_NOTE      = "syncNote";
    inline constexpr const char* INPUT_MODE     = "inputMode";
    inline constexpr const char* OUTPUT_GAIN    = "outputGain";
    inline constexpr const char* LIMITER_ON     = "limiterOn";
} // namespace ParamIDs

namespace ParamNames
{
    inline constexpr const char* TAIL_LENGTH    = "Tail Length";
    inline constexpr const char* SILENCE_GAP    = "Silence Gap";
    inline constexpr const char* SHAPE          = "Transient Shape";
    inline constexpr const char* INTENSITY      = "Intensity";
    inline constexpr const char* PITCH_SHIFT    = "Pitch Shift";
    inline constexpr const char* MIX            = "Mix";
    inline constexpr const char* SYNC_ENABLED   = "Sync to Host";
    inline constexpr const char* SYNC_NOTE      = "Sync Note Value";
    inline constexpr const char* INPUT_MODE     = "Input Mode";
    inline constexpr const char* OUTPUT_GAIN    = "Output Gain";
    inline constexpr const char* LIMITER_ON     = "Limiter";
} // namespace ParamNames

namespace ParamDefaults
{
    // Tail Length (ms)
    inline constexpr float TAIL_LENGTH_MIN     = 5.0f;
    inline constexpr float TAIL_LENGTH_MAX     = 5000.0f;
    inline constexpr float TAIL_LENGTH_DEFAULT = 50.0f;
    inline constexpr float TAIL_LENGTH_SKEW    = 0.4f;  // Skew toward lower values

    // Silence Gap (ms)
    inline constexpr float SILENCE_GAP_MIN     = 0.0f;
    inline constexpr float SILENCE_GAP_MAX     = 2000.0f;
    inline constexpr float SILENCE_GAP_DEFAULT = 100.0f;

    // Intensity (%)
    inline constexpr float INTENSITY_MIN       = 0.0f;
    inline constexpr float INTENSITY_MAX       = 100.0f;
    inline constexpr float INTENSITY_DEFAULT   = 75.0f;

    // Pitch Shift (semitones) — only active in Doppler mode
    inline constexpr float PITCH_SHIFT_MIN     = 0.0f;
    inline constexpr float PITCH_SHIFT_MAX     = 24.0f;
    inline constexpr float PITCH_SHIFT_DEFAULT = 12.0f;

    // Mix (Dry/Wet %)
    inline constexpr float MIX_MIN             = 0.0f;
    inline constexpr float MIX_MAX             = 100.0f;
    inline constexpr float MIX_DEFAULT         = 100.0f;

    // Sync to Host
    inline constexpr bool SYNC_ENABLED_DEFAULT = false;

    // Transient Shape (index into choice list)
    inline constexpr int SHAPE_DEFAULT         = 0;  // Exponential

    // Sync Note Value (index into choice list)
    inline constexpr int SYNC_NOTE_DEFAULT     = 2;  // 1/4 note

    // Input Mode (index into choice list)
    inline constexpr int INPUT_MODE_DEFAULT    = 0;  // External Audio

    // Output Gain (dB)
    inline constexpr float OUTPUT_GAIN_MIN     = -24.0f;
    inline constexpr float OUTPUT_GAIN_MAX     = 24.0f;
    inline constexpr float OUTPUT_GAIN_DEFAULT = 0.0f;

    // Limiter
    inline constexpr bool LIMITER_ON_DEFAULT   = true;

    // Parameter smoothing ramp time (seconds)
    inline constexpr double SMOOTHING_RAMP_SEC = 0.02;  // 20ms
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

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
