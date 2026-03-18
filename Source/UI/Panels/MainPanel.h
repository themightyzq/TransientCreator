#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Components/EnvelopeVisualizer.h"
#include "../Components/TransientControls.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

class MainPanel : public juce::Component
{
public:
    explicit MainPanel(juce::AudioProcessorValueTreeState& apvts);
    ~MainPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    TransientLookAndFeel lookAndFeel;
    EnvelopeVisualizer envelopeVisualizer;
    TransientControls transientControls;
};
