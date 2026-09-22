#include "PluginEditor.h"

TransientCreatorEditor::TransientCreatorEditor(TransientCreatorProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), mainPanel(p.apvts, p.sharedState)
{
    juce::ignoreUnused(processorRef);
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(mainPanel);
    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
    getConstrainer()->setFixedAspectRatio(700.0 / 550.0);
}

TransientCreatorEditor::~TransientCreatorEditor()
{
    setLookAndFeel(nullptr);
}

void TransientCreatorEditor::paint(juce::Graphics& g)
{
    // House chassis gradient (style guide section 2 / migration spec), in place of the old flat
    // colour literal.
    g.setGradientFill(zqsfx::ui::gradients::chassis(getLocalBounds().toFloat()));
    g.fillAll();
}

void TransientCreatorEditor::resized()
{
    mainPanel.setBounds(getLocalBounds());
}
