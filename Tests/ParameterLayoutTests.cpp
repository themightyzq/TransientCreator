// ParameterLayoutTests.cpp
// Guards the parameter-display contract: the unit lives ONLY in each float
// parameter's value text (via withStringFromValueFunction). A separate
// withLabel() would be appended a second time by hosts, producing "150 ms ms".

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include <array>

#include "Parameters/ParameterLayout.h"

namespace
{
    // Minimal AudioProcessor so an APVTS can be constructed to host the layout.
    class DummyProcessor : public juce::AudioProcessor
    {
    public:
        const juce::String getName() const override            { return "Dummy"; }
        void prepareToPlay (double, int) override               {}
        void releaseResources() override                        {}
        void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
        double getTailLengthSeconds() const override            { return 0.0; }
        bool acceptsMidi() const override                       { return false; }
        bool producesMidi() const override                      { return false; }
        juce::AudioProcessorEditor* createEditor() override     { return nullptr; }
        bool hasEditor() const override                         { return false; }
        int getNumPrograms() override                           { return 1; }
        int getCurrentProgram() override                        { return 0; }
        void setCurrentProgram (int) override                   {}
        const juce::String getProgramName (int) override        { return {}; }
        void changeProgramName (int, const juce::String&) override {}
        void getStateInformation (juce::MemoryBlock&) override  {}
        void setStateInformation (const void*, int) override    {}
    };

    struct FloatParam { const char* id; const char* unit; };

    // Every AudioParameterFloat whose value text already embeds its unit.
    const std::array<FloatParam, 11> floatParams { {
        { ParamIDs::TAIL_LENGTH,    "ms" },   // adaptive ms / s
        { ParamIDs::SILENCE_GAP,    "ms" },
        { ParamIDs::MIX,            "%"  },
        { ParamIDs::OUTPUT_GAIN,    "dB" },
        { ParamIDs::ATTACK_TIME,    "ms" },
        { ParamIDs::TRANSIENT_GAIN, "dB" },
        { ParamIDs::PITCH_START,    "st" },
        { ParamIDs::PITCH_END,      "st" },
        { ParamIDs::SINE_FREQ,      "Hz" },
        { ParamIDs::HUMANIZE,       "%"  },
        { ParamIDs::SUSTAIN_HOLD,   "%"  },
    } };
}

class ParameterLayoutTests : public juce::UnitTest
{
public:
    ParameterLayoutTests() : juce::UnitTest ("ParameterLayout", "DSP") {}

    void runTest() override
    {
        juce::ScopedJuceInitialiser_GUI juceInit;

        DummyProcessor proc;
        juce::AudioProcessorValueTreeState apvts (proc, nullptr, "PARAMS", createParameterLayout());

        beginTest ("Float params carry no separate label (unit lives in value text)");
        for (const auto& f : floatParams)
        {
            auto* p = apvts.getParameter (f.id);
            expect (p != nullptr, juce::String ("missing parameter ") + f.id);
            if (p == nullptr) continue;
            expect (p->getLabel().isEmpty(),
                    juce::String (f.id) + " should have an empty label, got \"" + p->getLabel() + "\"");
        }

        beginTest ("Displayed value includes the unit exactly once");
        for (const auto& f : floatParams)
        {
            auto* p = apvts.getParameter (f.id);
            if (p == nullptr) continue;

            // What a host renders: value text followed by the label.
            const juce::String shown = p->getCurrentValueAsText() + p->getLabel();
            const juce::String unit (f.unit);

            expect (shown.contains (unit),
                    juce::String (f.id) + " value text is missing its unit: \"" + shown + "\"");
            expect (shown.indexOf (unit) == shown.lastIndexOf (unit),
                    juce::String (f.id) + " unit appears more than once: \"" + shown + "\"");
        }
    }
};

static ParameterLayoutTests parameterLayoutTests;
