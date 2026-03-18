#include "TransientControls.h"
#include "../../DSP/EnvelopeGenerator.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

TransientControls::TransientControls(juce::AudioProcessorValueTreeState& apvts)
{
    shapeParam     = apvts.getRawParameterValue(ParamIDs::SHAPE);
    syncParam      = apvts.getRawParameterValue(ParamIDs::SYNC_ENABLED);
    inputModeParam = apvts.getRawParameterValue(ParamIDs::INPUT_MODE);

    // --- Timing knobs ---
    setupSlider(attackTimeSlider, attackTimeLabel, "Attack");
    attackTimeSlider.setTextValueSuffix(" ms");

    setupSlider(tailLengthSlider, tailLengthLabel, "Tail");
    tailLengthSlider.setTextValueSuffix(" ms");

    setupSlider(silenceGapSlider, silenceGapLabel, "Gap");
    silenceGapSlider.setTextValueSuffix(" ms");

    setupSlider(preDelaySlider, preDelayLabel, "Pre-Dly");
    preDelaySlider.setTextValueSuffix(" ms");

    // --- Shaping knobs ---
    setupSlider(intensitySlider, intensityLabel, "Intensity");
    intensitySlider.setTextValueSuffix(" %");

    setupSlider(transientGainSlider, transientGainLabel, "Boost");
    transientGainSlider.setTextValueSuffix(" dB");

    setupSlider(tensionSlider, tensionLabel, "Curve");

    setupSlider(mixSlider, mixLabel, "Mix");
    mixSlider.setTextValueSuffix(" %");

    setupSlider(pitchShiftSlider, pitchShiftLabel, "Pitch");
    pitchShiftSlider.setTextValueSuffix(" st");

    setupSlider(sineFreqSlider, sineFreqLabel, "Freq");
    sineFreqSlider.setTextValueSuffix(" Hz");

    // --- Output knobs ---
    setupSlider(hpfSlider, hpfLabel, "HPF");
    hpfSlider.setTextValueSuffix(" Hz");

    setupSlider(lpfSlider, lpfLabel, "LPF");
    lpfSlider.setTextValueSuffix(" Hz");

    setupSlider(outputGainSlider, outputGainLabel, "Gain");
    outputGainSlider.setTextValueSuffix(" dB");

    setupSlider(humanizeSlider, humanizeLabel, "Human.");
    humanizeSlider.setTextValueSuffix(" %");

    setupSlider(sustainHoldSlider, sustainHoldLabel, "Hold");
    sustainHoldSlider.setTextValueSuffix(" %");

    // --- Dropdowns ---
    shapeSelector.addItemList(shapeChoices, 1);
    addAndMakeVisible(shapeSelector);
    shapeLabel.setText("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(shapeLabel);

    inputModeSelector.addItemList(inputModeChoices, 1);
    addAndMakeVisible(inputModeSelector);
    inputModeLabel.setText("Input", juce::dontSendNotification);
    inputModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputModeLabel);

    dopplerDirSelector.addItemList(dopplerDirectionChoices, 1);
    addAndMakeVisible(dopplerDirSelector);
    dopplerDirLabel.setText("Direction", juce::dontSendNotification);
    dopplerDirLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dopplerDirLabel);

    syncNoteSelector.addItemList(syncNoteChoices, 1);
    addAndMakeVisible(syncNoteSelector);
    syncNoteLabel.setText("Note", juce::dontSendNotification);
    syncNoteLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(syncNoteLabel);

    // --- Toggles ---
    syncToggle.setButtonText("SYNC");
    addAndMakeVisible(syncToggle);
    limiterToggle.setButtonText("LIMIT");
    addAndMakeVisible(limiterToggle);

    // --- APVTS attachments (AFTER all components set up) ---
    tailLengthAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::TAIL_LENGTH, tailLengthSlider);
    silenceGapAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::SILENCE_GAP, silenceGapSlider);
    intensityAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::INTENSITY, intensitySlider);
    pitchShiftAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::PITCH_SHIFT, pitchShiftSlider);
    mixAttachment            = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::MIX, mixSlider);
    outputGainAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::OUTPUT_GAIN, outputGainSlider);
    attackTimeAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::ATTACK_TIME, attackTimeSlider);
    transientGainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::TRANSIENT_GAIN, transientGainSlider);
    tensionAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::ENVELOPE_TENSION, tensionSlider);
    hpfAttachment            = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::FILTER_HPF_FREQ, hpfSlider);
    lpfAttachment            = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::FILTER_LPF_FREQ, lpfSlider);
    sineFreqAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::SINE_FREQ, sineFreqSlider);
    preDelayAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::PRE_DELAY, preDelaySlider);
    humanizeAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::HUMANIZE, humanizeSlider);
    sustainHoldAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::SUSTAIN_HOLD, sustainHoldSlider);

    shapeAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ParamIDs::SHAPE, shapeSelector);
    syncNoteAttachment   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ParamIDs::SYNC_NOTE, syncNoteSelector);
    inputModeAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ParamIDs::INPUT_MODE, inputModeSelector);
    dopplerDirAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ParamIDs::DOPPLER_DIRECTION, dopplerDirSelector);

    syncAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, ParamIDs::SYNC_ENABLED, syncToggle);
    limiterAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, ParamIDs::LIMITER_ON, limiterToggle);

    // Initialize conditional state
    prevDopplerVisible  = static_cast<int>(shapeParam->load()) == static_cast<int>(EnvelopeShape::Doppler);
    prevSyncOn          = syncParam->load() >= 0.5f;
    prevSineFreqVisible = static_cast<int>(inputModeParam->load()) == 3;
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
    const bool isSyncOn  = syncParam->load() >= 0.5f;
    const bool isSine    = static_cast<int>(inputModeParam->load()) == 3;

    pitchShiftSlider.setVisible(isDoppler);
    pitchShiftLabel.setVisible(isDoppler);
    dopplerDirSelector.setVisible(isDoppler);
    dopplerDirLabel.setVisible(isDoppler);

    sineFreqSlider.setVisible(isSine);
    sineFreqLabel.setVisible(isSine);

    syncNoteSelector.setVisible(isSyncOn);
    syncNoteLabel.setVisible(isSyncOn);

    silenceGapSlider.setEnabled(!isSyncOn);
    silenceGapSlider.setAlpha(isSyncOn ? 0.4f : 1.0f);
    silenceGapLabel.setAlpha(isSyncOn ? 0.4f : 1.0f);

    const bool needsRelayout = (isDoppler != prevDopplerVisible)
                             || (isSyncOn != prevSyncOn)
                             || (isSine != prevSineFreqVisible);
    prevDopplerVisible  = isDoppler;
    prevSyncOn          = isSyncOn;
    prevSineFreqVisible = isSine;

    if (needsRelayout)
        resized();
}

void TransientControls::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(TransientLookAndFeel::BG_PANEL));

    // Section labels
    auto bounds = getLocalBounds().reduced(8);
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.6f));
    g.setFont(juce::FontOptions(10.0f));

    auto sectionY = bounds.getY() + 54;
    g.drawText("TIMING", bounds.getX(), sectionY, 50, 12, juce::Justification::centredLeft);

    sectionY += 78;
    g.drawText("SHAPE", bounds.getX(), sectionY, 50, 12, juce::Justification::centredLeft);

    sectionY += 78;
    g.drawText("OUTPUT", bounds.getX(), sectionY, 50, 12, juce::Justification::centredLeft);
}

void TransientControls::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    auto placeKnob = [](juce::Rectangle<int>& row, int knobWidth, juce::Slider& slider, juce::Label& label)
    {
        auto area = row.removeFromLeft(knobWidth).reduced(2);
        label.setBounds(area.removeFromTop(14));
        slider.setBounds(area);
    };

    // Row 1: Source & Shape dropdowns
    auto row1 = bounds.removeFromTop(48);
    {
        const int sectionCount = dopplerDirSelector.isVisible() ? 3 : 2;
        const int sectionW = row1.getWidth() / sectionCount;

        auto shapeArea = row1.removeFromLeft(sectionW).reduced(4);
        shapeLabel.setBounds(shapeArea.removeFromTop(14));
        shapeSelector.setBounds(shapeArea);

        auto inputArea = row1.removeFromLeft(sectionW).reduced(4);
        inputModeLabel.setBounds(inputArea.removeFromTop(14));
        inputModeSelector.setBounds(inputArea);

        if (dopplerDirSelector.isVisible())
        {
            auto dirArea = row1.reduced(4);
            dopplerDirLabel.setBounds(dirArea.removeFromTop(14));
            dopplerDirSelector.setBounds(dirArea);
        }
    }

    bounds.removeFromTop(16); // Space for section label

    // Row 2: Timing — Attack, Tail, Gap, Pre-Delay
    auto row2 = bounds.removeFromTop(72);
    {
        const int knobWidth = row2.getWidth() / 4;
        placeKnob(row2, knobWidth, attackTimeSlider, attackTimeLabel);
        placeKnob(row2, knobWidth, tailLengthSlider, tailLengthLabel);
        placeKnob(row2, knobWidth, silenceGapSlider, silenceGapLabel);
        placeKnob(row2, knobWidth, preDelaySlider, preDelayLabel);
    }

    bounds.removeFromTop(16); // Space for section label

    // Row 3: Shaping — Intensity, Boost, Curve, Hold, Mix [+ Pitch, Freq conditional]
    auto row3 = bounds.removeFromTop(72);
    {
        int numKnobs = 5;
        if (pitchShiftSlider.isVisible()) ++numKnobs;
        if (sineFreqSlider.isVisible()) ++numKnobs;
        const int knobWidth = row3.getWidth() / numKnobs;

        placeKnob(row3, knobWidth, intensitySlider, intensityLabel);
        placeKnob(row3, knobWidth, transientGainSlider, transientGainLabel);
        placeKnob(row3, knobWidth, tensionSlider, tensionLabel);
        placeKnob(row3, knobWidth, sustainHoldSlider, sustainHoldLabel);
        placeKnob(row3, knobWidth, mixSlider, mixLabel);
        if (pitchShiftSlider.isVisible())
            placeKnob(row3, knobWidth, pitchShiftSlider, pitchShiftLabel);
        if (sineFreqSlider.isVisible())
            placeKnob(row3, knobWidth, sineFreqSlider, sineFreqLabel);
    }

    bounds.removeFromTop(16); // Space for section label

    // Row 4: Output — HPF, LPF, Gain, Humanize
    auto row4 = bounds.removeFromTop(72);
    {
        const int knobWidth = row4.getWidth() / 4;
        placeKnob(row4, knobWidth, hpfSlider, hpfLabel);
        placeKnob(row4, knobWidth, lpfSlider, lpfLabel);
        placeKnob(row4, knobWidth, outputGainSlider, outputGainLabel);
        placeKnob(row4, knobWidth, humanizeSlider, humanizeLabel);
    }

    bounds.removeFromTop(4);

    // Row 5: Toggles — Sync, Note Value, Limiter
    auto row5 = bounds.removeFromTop(32);
    {
        auto syncArea = row5.removeFromLeft(70).reduced(2);
        syncToggle.setBounds(syncArea);

        if (syncNoteSelector.isVisible())
        {
            auto noteArea = row5.removeFromLeft(110).reduced(2);
            syncNoteLabel.setBounds(noteArea.removeFromTop(14));
            syncNoteSelector.setBounds(noteArea);
        }

        auto limiterArea = row5.removeFromRight(70).reduced(2);
        limiterToggle.setBounds(limiterArea);
    }
}
