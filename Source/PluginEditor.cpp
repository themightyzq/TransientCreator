#include "PluginEditor.h"

TransientCreatorEditor::TransientCreatorEditor(TransientCreatorProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    juce::ignoreUnused(processorRef);
    addAndMakeVisible(mainPanel);
    setSize(500, 350);
    setResizable(true, true);
    setResizeLimits(400, 280, 1200, 840);
    getConstrainer()->setFixedAspectRatio(500.0 / 350.0);
}

TransientCreatorEditor::~TransientCreatorEditor() = default;

void TransientCreatorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0f3460));
}

void TransientCreatorEditor::resized()
{
    mainPanel.setBounds(getLocalBounds());
}
