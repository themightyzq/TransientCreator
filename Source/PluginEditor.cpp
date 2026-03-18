#include "PluginEditor.h"

TransientCreatorEditor::TransientCreatorEditor(TransientCreatorProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), mainPanel(p.apvts)
{
    juce::ignoreUnused(processorRef);
    addAndMakeVisible(mainPanel);
    setSize(650, 420);
    setResizable(true, true);
    setResizeLimits(520, 336, 1300, 840);
    getConstrainer()->setFixedAspectRatio(650.0 / 420.0);
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
