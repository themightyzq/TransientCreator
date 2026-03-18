#include "ParameterLayout.h"

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Tail Length (ms) — skewed toward lower values for finer control of short transients
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::TAIL_LENGTH, 1 },
        ParamNames::TAIL_LENGTH,
        juce::NormalisableRange<float>(
            ParamDefaults::TAIL_LENGTH_MIN,
            ParamDefaults::TAIL_LENGTH_MAX,
            0.1f,
            ParamDefaults::TAIL_LENGTH_SKEW),
        ParamDefaults::TAIL_LENGTH_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Silence Gap (ms)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::SILENCE_GAP, 1 },
        ParamNames::SILENCE_GAP,
        juce::NormalisableRange<float>(
            ParamDefaults::SILENCE_GAP_MIN,
            ParamDefaults::SILENCE_GAP_MAX,
            0.1f),
        ParamDefaults::SILENCE_GAP_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Transient Shape — enum/choice parameter
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::SHAPE, 1 },
        ParamNames::SHAPE,
        shapeChoices,
        ParamDefaults::SHAPE_DEFAULT));

    // Intensity (%)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::INTENSITY, 1 },
        ParamNames::INTENSITY,
        juce::NormalisableRange<float>(
            ParamDefaults::INTENSITY_MIN,
            ParamDefaults::INTENSITY_MAX,
            0.1f),
        ParamDefaults::INTENSITY_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Pitch Shift (semitones) — only active in Doppler mode
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::PITCH_SHIFT, 1 },
        ParamNames::PITCH_SHIFT,
        juce::NormalisableRange<float>(
            ParamDefaults::PITCH_SHIFT_MIN,
            ParamDefaults::PITCH_SHIFT_MAX,
            0.1f),
        ParamDefaults::PITCH_SHIFT_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("st")));

    // Mix (Dry/Wet %)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::MIX, 1 },
        ParamNames::MIX,
        juce::NormalisableRange<float>(
            ParamDefaults::MIX_MIN,
            ParamDefaults::MIX_MAX,
            0.1f),
        ParamDefaults::MIX_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    // Sync to Host (bool)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::SYNC_ENABLED, 1 },
        ParamNames::SYNC_ENABLED,
        ParamDefaults::SYNC_ENABLED_DEFAULT));

    // Sync Note Value — choice parameter
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::SYNC_NOTE, 1 },
        ParamNames::SYNC_NOTE,
        syncNoteChoices,
        ParamDefaults::SYNC_NOTE_DEFAULT));

    // Input Mode — choice parameter
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::INPUT_MODE, 1 },
        ParamNames::INPUT_MODE,
        inputModeChoices,
        ParamDefaults::INPUT_MODE_DEFAULT));

    // Output Gain (dB)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::OUTPUT_GAIN, 1 },
        ParamNames::OUTPUT_GAIN,
        juce::NormalisableRange<float>(
            ParamDefaults::OUTPUT_GAIN_MIN,
            ParamDefaults::OUTPUT_GAIN_MAX,
            0.1f),
        ParamDefaults::OUTPUT_GAIN_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Limiter On/Off (bool)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { ParamIDs::LIMITER_ON, 1 },
        ParamNames::LIMITER_ON,
        ParamDefaults::LIMITER_ON_DEFAULT));

    // --- Phase 4 new parameters ---

    // Attack Time (ms) — 4A
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::ATTACK_TIME, 1 },
        ParamNames::ATTACK_TIME,
        juce::NormalisableRange<float>(
            ParamDefaults::ATTACK_TIME_MIN,
            ParamDefaults::ATTACK_TIME_MAX,
            0.01f,
            ParamDefaults::ATTACK_TIME_SKEW),
        ParamDefaults::ATTACK_TIME_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Transient Boost (dB) — 4B
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::TRANSIENT_GAIN, 1 },
        ParamNames::TRANSIENT_GAIN,
        juce::NormalisableRange<float>(
            ParamDefaults::TRANSIENT_GAIN_MIN,
            ParamDefaults::TRANSIENT_GAIN_MAX,
            0.1f),
        ParamDefaults::TRANSIENT_GAIN_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("dB")));

    // Envelope Tension / Curve — 4C
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::ENVELOPE_TENSION, 1 },
        ParamNames::ENVELOPE_TENSION,
        juce::NormalisableRange<float>(
            ParamDefaults::ENVELOPE_TENSION_MIN,
            ParamDefaults::ENVELOPE_TENSION_MAX,
            0.01f),
        ParamDefaults::ENVELOPE_TENSION_DEFAULT));

    // HPF Frequency (Hz) — 4D
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::FILTER_HPF_FREQ, 1 },
        ParamNames::FILTER_HPF_FREQ,
        juce::NormalisableRange<float>(
            ParamDefaults::HPF_FREQ_MIN,
            ParamDefaults::HPF_FREQ_MAX,
            0.1f,
            ParamDefaults::HPF_FREQ_SKEW),
        ParamDefaults::HPF_FREQ_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // LPF Frequency (Hz) — 4D
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::FILTER_LPF_FREQ, 1 },
        ParamNames::FILTER_LPF_FREQ,
        juce::NormalisableRange<float>(
            ParamDefaults::LPF_FREQ_MIN,
            ParamDefaults::LPF_FREQ_MAX,
            0.1f,
            ParamDefaults::LPF_FREQ_SKEW),
        ParamDefaults::LPF_FREQ_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // Sine Oscillator Frequency (Hz) — 4E
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::SINE_FREQ, 1 },
        ParamNames::SINE_FREQ,
        juce::NormalisableRange<float>(
            ParamDefaults::SINE_FREQ_MIN,
            ParamDefaults::SINE_FREQ_MAX,
            0.1f,
            ParamDefaults::SINE_FREQ_SKEW),
        ParamDefaults::SINE_FREQ_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("Hz")));

    // Doppler Direction — 4F
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::DOPPLER_DIRECTION, 1 },
        ParamNames::DOPPLER_DIRECTION,
        dopplerDirectionChoices,
        ParamDefaults::DOPPLER_DIRECTION_DEFAULT));

    // Pre-Delay (ms) — 4G
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::PRE_DELAY, 1 },
        ParamNames::PRE_DELAY,
        juce::NormalisableRange<float>(
            ParamDefaults::PRE_DELAY_MIN,
            ParamDefaults::PRE_DELAY_MAX,
            0.1f),
        ParamDefaults::PRE_DELAY_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    // Humanize (%) — 4H
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::HUMANIZE, 1 },
        ParamNames::HUMANIZE,
        juce::NormalisableRange<float>(
            ParamDefaults::HUMANIZE_MIN,
            ParamDefaults::HUMANIZE_MAX,
            0.1f),
        ParamDefaults::HUMANIZE_DEFAULT,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    return layout;
}
