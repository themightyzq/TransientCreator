#include "EnvelopeVisualizer.h"

void EnvelopeVisualizer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.drawRect(getLocalBounds(), 1);

    // Stub: draw envelope shape — implemented in Phase 6
    g.setColour(juce::Colours::grey);
    g.drawText("Envelope Visualizer", getLocalBounds(), juce::Justification::centred);
}

void EnvelopeVisualizer::resized()
{
    // Stub: layout — implemented in Phase 6
}
