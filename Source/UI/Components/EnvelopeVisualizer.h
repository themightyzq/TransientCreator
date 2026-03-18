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
    static constexpr int RESOLUTION       = 256;   // Number of points to draw
    static constexpr float TAIL_FRACTION  = 0.65f;  // Default proportion of display for tail vs gap
    static constexpr float REFERENCE_RATE = 44100.0f; // Reference sample rate for visualization

    // Lock-free parameter reads
    std::atomic<float>* shapeParam     = nullptr;
    std::atomic<float>* tailLengthParam = nullptr;
    std::atomic<float>* silenceGapParam = nullptr;
};
