#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../DSP/EnvelopeGenerator.h"
#include "../../Parameters/ParameterLayout.h"

class EnvelopeVisualizer : public juce::Component, private juce::Timer
{
public:
    explicit EnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts);
    ~EnvelopeVisualizer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    static constexpr int REPAINT_HZ       = 30;
    static constexpr int RESOLUTION       = 256;
    static constexpr float TAIL_FRACTION  = 0.65f;
    static constexpr float REFERENCE_RATE = 44100.0f;

    std::atomic<float>* shapeParam      = nullptr;
    std::atomic<float>* tailLengthParam = nullptr;
    std::atomic<float>* silenceGapParam = nullptr;
    std::atomic<float>* attackTimeParam = nullptr;
    std::atomic<float>* tensionParam    = nullptr;
};
