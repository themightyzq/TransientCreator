#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel/TransientLookAndFeel.h"
#include "UI/Panels/MainPanel.h"

class TransientCreatorEditor : public juce::AudioProcessorEditor
{
public:
    explicit TransientCreatorEditor(TransientCreatorProcessor&);
    ~TransientCreatorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    TransientCreatorProcessor& processorRef;

    // Installed at the EDITOR level (not on a child panel) so every descendant -- including the
    // TooltipWindow below and any AlertWindow (the LogoMark's About box) -- inherits the house
    // look, matching the LFlOw worked example. Declared before mainPanel so it is constructed
    // (and available) first.
    TransientLookAndFeel lookAndFeel;

    MainPanel mainPanel;
    juce::TooltipWindow tooltipWindow { this, 500 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientCreatorEditor)
};
