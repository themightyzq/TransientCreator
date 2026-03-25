#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Parameters/ParameterLayout.h"

class TransientControls : public juce::Component, private juce::Timer
{
public:
    explicit TransientControls(juce::AudioProcessorValueTreeState& apvts);
    ~TransientControls() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateConditionalState();
    void setupRotaryKnob(juce::Slider& slider, juce::Label& label,
                         const juce::String& labelText, double defaultValue);
    void setupVerticalFader(juce::Slider& slider, juce::Label& label,
                            const juce::String& labelText, double defaultValue);

    static constexpr int STATE_CHECK_HZ = 15;

    // Vertical faders
    juce::Slider attackTimeFader, sustainHoldFader, tailLengthFader;
    juce::Label attackTimeLabel, sustainHoldLabel, tailLengthLabel;

    // Primary knobs
    juce::Slider transientGainSlider, pitchStartSlider, pitchEndSlider;
    juce::Slider mixSlider, outputGainSlider;
    juce::Label transientGainLabel, pitchStartLabel, pitchEndLabel;
    juce::Label mixLabel, outputGainLabel;

    // Secondary knobs
    juce::Slider silenceGapSlider, humanizeSlider, sineFreqSlider;
    juce::Label silenceGapLabel, humanizeLabel, sineFreqLabel;

    // Dropdowns
    juce::ComboBox inputModeSelector, syncNoteSelector;
    juce::Label inputModeLabel, syncNoteLabel;

    // Toggles
    juce::ToggleButton syncToggle, limiterToggle;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainHoldAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tailLengthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transientGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchStartAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchEndAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> silenceGapAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> humanizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sineFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inputModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterAttachment;

    // Conditional state
    std::atomic<float>* syncParam      = nullptr;
    std::atomic<float>* inputModeParam = nullptr;
    bool prevSyncOn          = false;
    bool prevSineFreqVisible = false;

    // Set in resized(), used in paint() for dynamic group label positioning
    int knobStackTopY = 0;
};
