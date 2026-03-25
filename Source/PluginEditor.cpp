#include "PluginEditor.h"

TransientCreatorEditor::TransientCreatorEditor(TransientCreatorProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), mainPanel(p.apvts, p.sharedState)
{
    juce::ignoreUnused(processorRef);
    addAndMakeVisible(mainPanel);
    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
    getConstrainer()->setFixedAspectRatio(700.0 / 550.0);
}

TransientCreatorEditor::~TransientCreatorEditor() = default;

void TransientCreatorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0e1a));
}

void TransientCreatorEditor::resized()
{
    mainPanel.setBounds(getLocalBounds());
}
