#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class EnvelopeVisualizer : public juce::Component
{
public:
    EnvelopeVisualizer() = default;
    ~EnvelopeVisualizer() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
};
