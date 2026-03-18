#include "TransientControls.h"
#include "../../DSP/EnvelopeGenerator.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

TransientControls::TransientControls(juce::AudioProcessorValueTreeState& apvts)
{
    // Cache atomic pointers for conditional visibility checks
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

    // --- Sync toggle ---
    syncToggle.setButtonText("SYNC");
    addAndMakeVisible(syncToggle);

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

    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::SHAPE, shapeSelector);
    syncNoteAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::SYNC_NOTE, syncNoteSelector);
    inputModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::INPUT_MODE, inputModeSelector);

    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, ParamIDs::SYNC_ENABLED, syncToggle);

    // Start polling for conditional visibility
    updateConditionalVisibility();
    startTimerHz(VISIBILITY_CHECK_HZ);
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
    updateConditionalVisibility();
}

void TransientControls::updateConditionalVisibility()
{
    const bool isDoppler = static_cast<int>(shapeParam->load()) == static_cast<int>(EnvelopeShape::Doppler);
    const bool isSyncOn = syncParam->load() >= 0.5f;

    pitchShiftSlider.setVisible(isDoppler);
    pitchShiftLabel.setVisible(isDoppler);

    syncNoteSelector.setVisible(isSyncOn);
    syncNoteLabel.setVisible(isSyncOn);
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

    // Middle row: Main knobs (Tail, Gap, Intensity, Pitch, Mix)
    auto knobRow = bounds.removeFromTop(90);
    {
        const bool pitchVisible = pitchShiftSlider.isVisible();
        const int numKnobs = pitchVisible ? 5 : 4;
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
    }

    bounds.removeFromTop(4);

    // Bottom row: Sync toggle + Note Value
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
    }
}
