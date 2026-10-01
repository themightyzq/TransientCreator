// DefaultsTests.cpp
// Guards two owner-approved defaults:
//   1. A new instance's Input Mode is External Audio (pass-through of the
//      audio already playing), not an internal noise/sine generator.
//   2. The default custom curve starts at full scale and decays to exactly
//      zero at its end point.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>

#include "Parameters/ParameterLayout.h"
#include "SharedState.h"

namespace
{
    class DefaultsDummyProcessor : public juce::AudioProcessor
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
}

class DefaultsTests : public juce::UnitTest
{
public:
    DefaultsTests() : juce::UnitTest ("Defaults", "DSP") {}

    void runTest() override
    {
        juce::ScopedJuceInitialiser_GUI juceInit;

        beginTest ("inputMode parameter defaults to External Audio");
        {
            DefaultsDummyProcessor proc;
            juce::AudioProcessorValueTreeState apvts (proc, nullptr, "PARAMS", createParameterLayout());

            expectEquals (ParamDefaults::INPUT_MODE_DEFAULT, 0);
            expectEquals (inputModeChoices[0], juce::String ("External Audio"));

            auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (ParamIDs::INPUT_MODE));
            expect (choice != nullptr, "inputMode should be an AudioParameterChoice");
            if (choice != nullptr)
            {
                expectEquals (choice->getIndex(), 0);
                expectEquals (choice->getCurrentChoiceName(), juce::String ("External Audio"));

                // Init resets every parameter to getDefaultValue(): that must also be External.
                choice->setValueNotifyingHost (choice->convertTo0to1 (1.0f));   // move off default
                expectEquals (choice->getIndex(), 1);
                choice->setValueNotifyingHost (apvts.getParameter (ParamIDs::INPUT_MODE)->getDefaultValue());
                expectEquals (choice->getIndex(), 0);
            }
        }

        beginTest ("Default custom curve runs from 1 down to exactly 0");
        {
            SharedUIState state;
            expectEquals (state.customCurveStaging.front(), 1.0f);
            expectEquals (state.customCurveStaging.back(), 0.0f);
            expectEquals (state.customCurveDisplay.front(), 1.0f);
            expectEquals (state.customCurveDisplay.back(), 0.0f);

            for (float v : state.customCurveStaging)
                expect (v >= 0.0f && v <= 1.0f, "default curve sample out of [0, 1]");

            expectEquals (state.breakpoints.back().x, 1.0f);
            expectEquals (state.breakpoints.back().y, 0.0f);
        }
    }
};

static DefaultsTests defaultsTests;
