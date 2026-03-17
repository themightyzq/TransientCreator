#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
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
    MainPanel mainPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientCreatorEditor)
};
