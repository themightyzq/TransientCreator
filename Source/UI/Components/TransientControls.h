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

    // Helper to create a labeled rotary knob
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    static constexpr int STATE_CHECK_HZ = 15;

    // Knobs
    juce::Slider tailLengthSlider;
    juce::Slider silenceGapSlider;
    juce::Slider intensitySlider;
    juce::Slider pitchShiftSlider;
    juce::Slider mixSlider;
    juce::Slider outputGainSlider;

    // Labels
    juce::Label tailLengthLabel;
    juce::Label silenceGapLabel;
    juce::Label intensityLabel;
    juce::Label pitchShiftLabel;
    juce::Label mixLabel;
    juce::Label outputGainLabel;
    juce::Label shapeLabel;
    juce::Label syncNoteLabel;
    juce::Label inputModeLabel;

    // Dropdown selectors
    juce::ComboBox shapeSelector;
    juce::ComboBox syncNoteSelector;
    juce::ComboBox inputModeSelector;

    // Toggles
    juce::ToggleButton syncToggle;
    juce::ToggleButton limiterToggle;

    // APVTS attachments (must be declared AFTER the components they attach to)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tailLengthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> silenceGapAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchShiftAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inputModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterAttachment;

    // For conditional state checks
    std::atomic<float>* shapeParam = nullptr;
    std::atomic<float>* syncParam  = nullptr;

    // Track previous state to trigger relayout only on change
    bool prevPitchVisible = false;
    bool prevSyncOn = false;
};
