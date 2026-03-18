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
    float computeEnvelopeAt(float normalizedPos, EnvelopeShape shape, float tailSamples) const;

    static constexpr int REPAINT_HZ       = 30;
    static constexpr int RESOLUTION       = 256;   // Number of points to draw
    static constexpr float TAIL_FRACTION  = 0.65f;  // Proportion of display for tail vs gap

    // Envelope math constants (duplicated from EnvelopeGenerator for UI-only computation)
    static constexpr float ENVELOPE_THRESHOLD    = 0.001f;
    static constexpr float LOG_CURVATURE_K       = 10.0f;
    static constexpr float GAUSSIAN_SIGMA_RATIO  = 0.25f;
    static constexpr float DOUBLE_TAP_SPACING    = 0.3f;
    static constexpr float PERCUSSIVE_ATTACK_RATIO = 0.02f;  // Approximate 1ms as fraction
    static constexpr float PERCUSSIVE_BODY_RATIO   = 0.15f;
    static constexpr float PERCUSSIVE_BODY_DROP    = 0.3f;

    // Lock-free parameter reads
    std::atomic<float>* shapeParam     = nullptr;
    std::atomic<float>* tailLengthParam = nullptr;
    std::atomic<float>* silenceGapParam = nullptr;
};
