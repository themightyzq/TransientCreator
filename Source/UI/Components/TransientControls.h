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
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    static constexpr int STATE_CHECK_HZ = 15;

    // --- Existing knobs ---
    juce::Slider tailLengthSlider, silenceGapSlider, intensitySlider;
    juce::Slider pitchShiftSlider, mixSlider, outputGainSlider;
    // --- Phase 4 new knobs ---
    juce::Slider attackTimeSlider, transientGainSlider, tensionSlider;
    juce::Slider hpfSlider, lpfSlider, sineFreqSlider;
    juce::Slider preDelaySlider, humanizeSlider, sustainHoldSlider;

    // --- Labels ---
    juce::Label tailLengthLabel, silenceGapLabel, intensityLabel;
    juce::Label pitchShiftLabel, mixLabel, outputGainLabel;
    juce::Label shapeLabel, syncNoteLabel, inputModeLabel;
    juce::Label attackTimeLabel, transientGainLabel, tensionLabel;
    juce::Label hpfLabel, lpfLabel, sineFreqLabel;
    juce::Label dopplerDirLabel, preDelayLabel, humanizeLabel, sustainHoldLabel;

    // --- Dropdowns ---
    juce::ComboBox shapeSelector, syncNoteSelector, inputModeSelector;
    juce::ComboBox dopplerDirSelector;

    // --- Toggles ---
    juce::ToggleButton syncToggle, limiterToggle;

    // --- APVTS attachments (AFTER components) ---
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tailLengthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> silenceGapAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intensityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchShiftAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> transientGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tensionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lpfAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sineFreqAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> preDelayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> humanizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainHoldAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> shapeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> syncNoteAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> inputModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> dopplerDirAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> limiterAttachment;

    // --- Conditional state ---
    std::atomic<float>* shapeParam     = nullptr;
    std::atomic<float>* syncParam      = nullptr;
    std::atomic<float>* inputModeParam = nullptr;

    bool prevDopplerVisible  = false;
    bool prevSyncOn          = false;
    bool prevSineFreqVisible = false;
};
