#include "MainPanel.h"

MainPanel::MainPanel()
{
    addAndMakeVisible(envelopeVisualizer);
    addAndMakeVisible(transientControls);
}

void MainPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff16213e));
}

void MainPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    auto topArea = bounds.removeFromTop(bounds.getHeight() / 3);

    envelopeVisualizer.setBounds(topArea);
    transientControls.setBounds(bounds);
}
