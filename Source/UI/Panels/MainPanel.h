#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Components/EnvelopeVisualizer.h"
#include "../Components/TransientControls.h"
#include "../LookAndFeel/TransientLookAndFeel.h"
#include "../../SharedState.h"
#include "../../Parameters/ParameterLayout.h"

class MainPanel : public juce::Component
{
public:
    MainPanel(juce::AudioProcessorValueTreeState& apvts, SharedUIState& sharedState);
    ~MainPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvtsRef;
    TransientLookAndFeel lookAndFeel;
    EnvelopeVisualizer envelopeVisualizer;
    TransientControls transientControls;

    // Shape selector bar (below curve editor)
    juce::ComboBox shapeSelector;
    juce::TextButton shapePrevButton { "<" };
    juce::TextButton shapeNextButton { ">" };
    juce::Label shapeBarLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    std::atomic<float>* shapeParam = nullptr;
};
