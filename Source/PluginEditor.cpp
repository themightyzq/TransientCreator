#include "PluginEditor.h"

TransientCreatorEditor::TransientCreatorEditor(TransientCreatorProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), mainPanel(p.apvts, p.sharedState, p.presetManager)
{
    juce::ignoreUnused(processorRef);
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(mainPanel);

    // Read any saved size BEFORE the default setSize() below -- resized() writes the current
    // size back to the processor on every call, so reading first avoids clobbering a restored
    // size with the 700x550 default.
    const int storedWidth = processorRef.getEditorWidth();
    const int storedHeight = processorRef.getEditorHeight();

    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
    getConstrainer()->setFixedAspectRatio(700.0 / 550.0);

    if (storedWidth > 0 && storedHeight > 0
        && storedWidth >= getConstrainer()->getMinimumWidth()
        && storedWidth <= getConstrainer()->getMaximumWidth()
        && storedHeight >= getConstrainer()->getMinimumHeight()
        && storedHeight <= getConstrainer()->getMaximumHeight())
    {
        setSize(storedWidth, storedHeight);
    }
    else
    {
        setSize(700, 550);
    }
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

    // Persist the current size (message-thread only) so the next session can restore it --
    // see PluginProcessor::getStateInformation()/setStateInformation().
    processorRef.setEditorWidth(getWidth());
    processorRef.setEditorHeight(getHeight());
}
