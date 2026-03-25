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
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float v, int) -> juce::String {
                if (v >= 1000.0f) return juce::String(v / 1000.0f, 2) + " s";
                if (v < 10.0f)    return juce::String(v, 2) + " ms";
                if (v < 100.0f)   return juce::String(v, 1) + " ms";
                return juce::String(static_cast<int>(v)) + " ms";
            })));

    // Silence Gap (ms)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::SILENCE_GAP, 1 },
        ParamNames::SILENCE_GAP,
        juce::NormalisableRange<float>(
            ParamDefaults::SILENCE_GAP_MIN,
            ParamDefaults::SILENCE_GAP_MAX,
            0.1f),
        ParamDefaults::SILENCE_GAP_DEFAULT,
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float v, int) -> juce::String {
                if (v >= 1000.0f) return juce::String(v / 1000.0f, 2) + " s";
                if (v < 10.0f)    return juce::String(v, 1) + " ms";
                if (v < 100.0f)   return juce::String(v, 1) + " ms";
                return juce::String(static_cast<int>(v)) + " ms";
            })));

    // Transient Shape — enum/choice parameter
    layout.add(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ParamIDs::SHAPE, 1 },
        ParamNames::SHAPE,
        shapeChoices,
        ParamDefaults::SHAPE_DEFAULT));

    // Mix (Dry/Wet %)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::MIX, 1 },
        ParamNames::MIX,
        juce::NormalisableRange<float>(
            ParamDefaults::MIX_MIN,
            ParamDefaults::MIX_MAX,
            0.1f),
        ParamDefaults::MIX_DEFAULT,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " %"; })));

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
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " dB"; })));

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
        juce::AudioParameterFloatAttributes()
            .withLabel("ms")
            .withStringFromValueFunction([](float v, int) -> juce::String {
                if (v >= 1000.0f) return juce::String(v / 1000.0f, 2) + " s";
                if (v < 1.0f)     return juce::String(v, 2) + " ms";
                if (v < 100.0f)   return juce::String(v, 1) + " ms";
                return juce::String(static_cast<int>(v)) + " ms";
            })));

    // Transient Boost (dB) — 4B
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::TRANSIENT_GAIN, 1 },
        ParamNames::TRANSIENT_GAIN,
        juce::NormalisableRange<float>(
            ParamDefaults::TRANSIENT_GAIN_MIN,
            ParamDefaults::TRANSIENT_GAIN_MAX,
            0.1f),
        ParamDefaults::TRANSIENT_GAIN_DEFAULT,
        juce::AudioParameterFloatAttributes()
            .withLabel("dB")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " dB"; })));

    // Pitch Start (semitones)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::PITCH_START, 1 },
        ParamNames::PITCH_START,
        juce::NormalisableRange<float>(
            ParamDefaults::PITCH_START_MIN,
            ParamDefaults::PITCH_START_MAX,
            0.1f),
        ParamDefaults::PITCH_START_DEFAULT,
        juce::AudioParameterFloatAttributes()
            .withLabel("st")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " st"; })));

    // Pitch End (semitones)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::PITCH_END, 1 },
        ParamNames::PITCH_END,
        juce::NormalisableRange<float>(
            ParamDefaults::PITCH_END_MIN,
            ParamDefaults::PITCH_END_MAX,
            0.1f),
        ParamDefaults::PITCH_END_DEFAULT,
        juce::AudioParameterFloatAttributes()
            .withLabel("st")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " st"; })));

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
        juce::AudioParameterFloatAttributes()
            .withLabel("Hz")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " Hz"; })));

    // Humanize (%) — 4H
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::HUMANIZE, 1 },
        ParamNames::HUMANIZE,
        juce::NormalisableRange<float>(
            ParamDefaults::HUMANIZE_MIN,
            ParamDefaults::HUMANIZE_MAX,
            0.1f),
        ParamDefaults::HUMANIZE_DEFAULT,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " %"; })));

    // Sustain Hold (%) — 7A
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ParamIDs::SUSTAIN_HOLD, 1 },
        ParamNames::SUSTAIN_HOLD,
        juce::NormalisableRange<float>(
            ParamDefaults::SUSTAIN_HOLD_MIN,
            ParamDefaults::SUSTAIN_HOLD_MAX,
            0.1f),
        ParamDefaults::SUSTAIN_HOLD_DEFAULT,
        juce::AudioParameterFloatAttributes()
            .withLabel("%")
            .withStringFromValueFunction([](float v, int) { return juce::String(v, 1) + " %"; })));

    return layout;
}
