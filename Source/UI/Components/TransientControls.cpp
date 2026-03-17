#include "TransientControls.h"

void TransientControls::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Stub: draw controls — implemented in Phase 6
    g.setColour(juce::Colours::grey);
    g.drawText("Controls", getLocalBounds(), juce::Justification::centred);
}

void TransientControls::resized()
{
    // Stub: layout controls — implemented in Phase 6
}
