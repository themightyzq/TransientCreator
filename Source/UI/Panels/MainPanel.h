#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <zqsfx_ui/zqsfx_ui.h>
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
    void showAboutBox();

    juce::AudioProcessorValueTreeState& apvtsRef;
    // The LookAndFeel itself now lives on the editor (TransientCreatorEditor::lookAndFeel) so
    // it also covers the TooltipWindow and the LogoMark's About-box AlertWindow -- see
    // PluginEditor.h for the rationale.
    EnvelopeVisualizer envelopeVisualizer;
    TransientControls transientControls;

    // The ZQ SFX mark (style guide section 5): header row, far right; also the About-box
    // trigger, replacing the old plain-text "ZQ SFX" caption.
    zqsfx::ui::LogoMark logo { "Transient Creator" };

    // Shape selector bar (below curve editor)
    juce::ComboBox shapeSelector;
    juce::TextButton shapePrevButton { "<" };
    juce::TextButton shapeNextButton { ">" };
    juce::Label shapeBarLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    std::atomic<float>* shapeParam = nullptr;
};
