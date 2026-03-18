#include "MainPanel.h"

MainPanel::MainPanel(juce::AudioProcessorValueTreeState& apvts)
    : envelopeVisualizer(apvts),
      transientControls(apvts)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(envelopeVisualizer);
    addAndMakeVisible(transientControls);
}

MainPanel::~MainPanel()
{
    setLookAndFeel(nullptr);
}

void MainPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(TransientLookAndFeel::BG_DARK));
}

void MainPanel::resized()
{
    auto bounds = getLocalBounds().reduced(6);

    // Top 35%: envelope visualizer
    auto vizHeight = static_cast<int>(bounds.getHeight() * 0.35f);
    envelopeVisualizer.setBounds(bounds.removeFromTop(vizHeight));

    bounds.removeFromTop(4);

    // Remaining: controls
    transientControls.setBounds(bounds);
}
