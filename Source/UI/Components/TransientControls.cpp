#include "TransientControls.h"
#include "../../DSP/EnvelopeGenerator.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

TransientControls::TransientControls(juce::AudioProcessorValueTreeState& apvts)
{
    // Cache atomic pointers for conditional state checks
    shapeParam = apvts.getRawParameterValue(ParamIDs::SHAPE);
    syncParam  = apvts.getRawParameterValue(ParamIDs::SYNC_ENABLED);

    // --- Knobs ---
    setupSlider(tailLengthSlider, tailLengthLabel, "Tail");
    tailLengthSlider.setTextValueSuffix(" ms");

    setupSlider(silenceGapSlider, silenceGapLabel, "Gap");
    silenceGapSlider.setTextValueSuffix(" ms");

    setupSlider(intensitySlider, intensityLabel, "Intensity");
    intensitySlider.setTextValueSuffix(" %");

    setupSlider(pitchShiftSlider, pitchShiftLabel, "Pitch");
    pitchShiftSlider.setTextValueSuffix(" st");

    setupSlider(mixSlider, mixLabel, "Mix");
    mixSlider.setTextValueSuffix(" %");

    setupSlider(outputGainSlider, outputGainLabel, "Gain");
    outputGainSlider.setTextValueSuffix(" dB");

    // --- Dropdowns ---
    shapeSelector.addItemList(shapeChoices, 1);
    addAndMakeVisible(shapeSelector);
    shapeLabel.setText("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(shapeLabel);

    syncNoteSelector.addItemList(syncNoteChoices, 1);
    addAndMakeVisible(syncNoteSelector);
    syncNoteLabel.setText("Note", juce::dontSendNotification);
    syncNoteLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(syncNoteLabel);

    inputModeSelector.addItemList(inputModeChoices, 1);
    addAndMakeVisible(inputModeSelector);
    inputModeLabel.setText("Input", juce::dontSendNotification);
    inputModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputModeLabel);

    // --- Toggles ---
    syncToggle.setButtonText("SYNC");
    addAndMakeVisible(syncToggle);

    limiterToggle.setButtonText("LIMIT");
    addAndMakeVisible(limiterToggle);

    // --- APVTS attachments (must be created AFTER components are set up) ---
    tailLengthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParamIDs::TAIL_LENGTH, tailLengthSlider);
    silenceGapAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParamIDs::SILENCE_GAP, silenceGapSlider);
    intensityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParamIDs::INTENSITY, intensitySlider);
    pitchShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParamIDs::PITCH_SHIFT, pitchShiftSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParamIDs::MIX, mixSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, ParamIDs::OUTPUT_GAIN, outputGainSlider);

    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::SHAPE, shapeSelector);
    syncNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::SYNC_NOTE, syncNoteSelector);
    inputModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::INPUT_MODE, inputModeSelector);

    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::SYNC_ENABLED, syncToggle);
    limiterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::LIMITER_ON, limiterToggle);

    // Initialize conditional state
    prevPitchVisible = static_cast<int>(shapeParam->load()) == static_cast<int>(EnvelopeShape::Doppler);
    prevSyncOn = syncParam->load() >= 0.5f;
    updateConditionalState();
    startTimerHz(STATE_CHECK_HZ);
}

TransientControls::~TransientControls()
{
    stopTimer();
}

void TransientControls::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& labelText)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 16);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void TransientControls::timerCallback()
{
    updateConditionalState();
}

void TransientControls::updateConditionalState()
{
    const bool isDoppler = static_cast<int>(shapeParam->load()) == static_cast<int>(EnvelopeShape::Doppler);
    const bool isSyncOn = syncParam->load() >= 0.5f;

    // Pitch knob: visible only in Doppler mode
    pitchShiftSlider.setVisible(isDoppler);
    pitchShiftLabel.setVisible(isDoppler);

    // Note selector: visible only when Sync is ON
    syncNoteSelector.setVisible(isSyncOn);
    syncNoteLabel.setVisible(isSyncOn);

    // Gap knob: disabled (greyed out) when Sync is ON — gap is auto-calculated from tempo
    silenceGapSlider.setEnabled(!isSyncOn);
    silenceGapSlider.setAlpha(isSyncOn ? 0.4f : 1.0f);
    silenceGapLabel.setAlpha(isSyncOn ? 0.4f : 1.0f);

    // Trigger relayout if visibility/state changed
    const bool needsRelayout = (isDoppler != prevPitchVisible) || (isSyncOn != prevSyncOn);
    prevPitchVisible = isDoppler;
    prevSyncOn = isSyncOn;

    if (needsRelayout)
        resized();
}

void TransientControls::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(TransientLookAndFeel::BG_PANEL));
}

void TransientControls::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Top row: Shape selector + Input Mode selector
    auto topRow = bounds.removeFromTop(50);
    {
        auto shapeArea = topRow.removeFromLeft(topRow.getWidth() / 2).reduced(4);
        shapeLabel.setBounds(shapeArea.removeFromTop(16));
        shapeSelector.setBounds(shapeArea);

        auto inputArea = topRow.reduced(4);
        inputModeLabel.setBounds(inputArea.removeFromTop(16));
        inputModeSelector.setBounds(inputArea);
    }

    bounds.removeFromTop(4);

    // Middle row: Main knobs — always show Tail, Gap, Intensity, Mix, Gain; conditionally Pitch
    auto knobRow = bounds.removeFromTop(90);
    {
        const bool pitchVisible = pitchShiftSlider.isVisible();
        const int numKnobs = pitchVisible ? 6 : 5;
        const int knobWidth = knobRow.getWidth() / numKnobs;

        auto placeKnob = [&](juce::Slider& slider, juce::Label& label)
        {
            auto area = knobRow.removeFromLeft(knobWidth).reduced(2);
            label.setBounds(area.removeFromTop(16));
            slider.setBounds(area);
        };

        placeKnob(tailLengthSlider, tailLengthLabel);
        placeKnob(silenceGapSlider, silenceGapLabel);
        placeKnob(intensitySlider, intensityLabel);
        if (pitchVisible)
            placeKnob(pitchShiftSlider, pitchShiftLabel);
        placeKnob(mixSlider, mixLabel);
        placeKnob(outputGainSlider, outputGainLabel);
    }

    bounds.removeFromTop(4);

    // Bottom row: Sync toggle + Note Value + Limiter toggle
    auto bottomRow = bounds.removeFromTop(40);
    {
        auto syncArea = bottomRow.removeFromLeft(80).reduced(4);
        syncToggle.setBounds(syncArea);

        if (syncNoteSelector.isVisible())
        {
            auto noteArea = bottomRow.removeFromLeft(120).reduced(4);
            syncNoteLabel.setBounds(noteArea.removeFromTop(16));
            syncNoteSelector.setBounds(noteArea);
        }

        // Limiter toggle on the right side
        auto limiterArea = bottomRow.removeFromRight(80).reduced(4);
        limiterToggle.setBounds(limiterArea);
    }
}
