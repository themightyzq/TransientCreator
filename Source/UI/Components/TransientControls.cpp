#include "TransientControls.h"
#include "../../DSP/EnvelopeGenerator.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

TransientControls::TransientControls(juce::AudioProcessorValueTreeState& apvts)
{
    shapeParam     = apvts.getRawParameterValue(ParamIDs::SHAPE);
    syncParam      = apvts.getRawParameterValue(ParamIDs::SYNC_ENABLED);
    inputModeParam = apvts.getRawParameterValue(ParamIDs::INPUT_MODE);

    // --- Section labels ---
    auto setupSectionLabel = [&](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::FontOptions(11.0f));
        label.setColour(juce::Label::textColourId,
                        juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.6f));
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);
    };

    setupSectionLabel(timingSectionLabel, "TIMING");
    setupSectionLabel(shapeSectionLabel, "SHAPE");
    setupSectionLabel(outputSectionLabel, "OUTPUT");

    // --- Timing knobs ---
    setupSlider(attackTimeSlider, attackTimeLabel, "Attack", ParamDefaults::ATTACK_TIME_DEFAULT);
    attackTimeSlider.setTextValueSuffix(" ms");
    attackTimeSlider.setTooltip("Onset ramp time — 0 ms is instant apex, higher values soften the attack");

    setupSlider(tailLengthSlider, tailLengthLabel, "Tail", ParamDefaults::TAIL_LENGTH_DEFAULT);
    tailLengthSlider.setTextValueSuffix(" ms");
    tailLengthSlider.setTooltip("Duration of the transient decay in milliseconds");

    setupSlider(silenceGapSlider, silenceGapLabel, "Gap", ParamDefaults::SILENCE_GAP_DEFAULT);
    silenceGapSlider.setTextValueSuffix(" ms");
    silenceGapSlider.setTooltip("Silence between transients (overridden when Sync is ON)");

    setupSlider(preDelaySlider, preDelayLabel, "Pre-Dly", ParamDefaults::PRE_DELAY_DEFAULT);
    preDelaySlider.setTextValueSuffix(" ms");
    preDelaySlider.setTooltip("Offset transient timing for groove shaping");

    // --- Shaping knobs ---
    setupSlider(intensitySlider, intensityLabel, "Intensity", ParamDefaults::INTENSITY_DEFAULT);
    intensitySlider.setTextValueSuffix(" %");
    intensitySlider.setTooltip("How strongly the envelope reshapes the audio (0% = passthrough)");

    setupSlider(transientGainSlider, transientGainLabel, "Boost", ParamDefaults::TRANSIENT_GAIN_DEFAULT);
    transientGainSlider.setTextValueSuffix(" dB");
    transientGainSlider.setTooltip("Amplify the transient peak — scales with envelope (0 dB = no boost)");

    setupSlider(tensionSlider, tensionLabel, "Curve", ParamDefaults::ENVELOPE_TENSION_DEFAULT);
    tensionSlider.setTooltip("Warp the envelope curve: < 1.0 = gentle decay, > 1.0 = snappy punch");

    setupSlider(sustainHoldSlider, sustainHoldLabel, "Hold", ParamDefaults::SUSTAIN_HOLD_DEFAULT);
    sustainHoldSlider.setTextValueSuffix(" %");
    sustainHoldSlider.setTooltip("Hold at peak amplitude before decay begins (% of tail duration)");

    setupSlider(mixSlider, mixLabel, "Mix", ParamDefaults::MIX_DEFAULT);
    mixSlider.setTextValueSuffix(" %");
    mixSlider.setTooltip("Blend between dry input (0%) and processed transient output (100%)");

    setupSlider(pitchShiftSlider, pitchShiftLabel, "Pitch", ParamDefaults::PITCH_SHIFT_DEFAULT);
    pitchShiftSlider.setTextValueSuffix(" st");
    pitchShiftSlider.setTooltip("Doppler pitch drop in semitones over the tail duration");

    setupSlider(sineFreqSlider, sineFreqLabel, "Freq", ParamDefaults::SINE_FREQ_DEFAULT);
    sineFreqSlider.setTextValueSuffix(" Hz");
    sineFreqSlider.setTooltip("Frequency of the internal sine oscillator");

    // --- Output knobs ---
    setupSlider(hpfSlider, hpfLabel, "HPF", ParamDefaults::HPF_FREQ_DEFAULT);
    hpfSlider.setTextValueSuffix(" Hz");
    hpfSlider.setTooltip("High-pass filter on the wet signal — removes low frequencies");

    setupSlider(lpfSlider, lpfLabel, "LPF", ParamDefaults::LPF_FREQ_DEFAULT);
    lpfSlider.setTextValueSuffix(" Hz");
    lpfSlider.setTooltip("Low-pass filter on the wet signal — removes high frequencies");

    setupSlider(outputGainSlider, outputGainLabel, "Gain", ParamDefaults::OUTPUT_GAIN_DEFAULT);
    outputGainSlider.setTextValueSuffix(" dB");
    outputGainSlider.setTooltip("Output level boost or cut in dB");

    setupSlider(humanizeSlider, humanizeLabel, "Human.", ParamDefaults::HUMANIZE_DEFAULT);
    humanizeSlider.setTextValueSuffix(" %");
    humanizeSlider.setTooltip("Per-cycle random variation on timing for organic feel");

    // --- Dropdowns ---
    shapeSelector.addItemList(shapeChoices, 1);
    shapeSelector.setTooltip("Envelope curve shape for the transient");
    shapeSelector.setTitle("Shape");
    addAndMakeVisible(shapeSelector);
    shapeLabel.setText("Shape", juce::dontSendNotification);
    shapeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(shapeLabel);

    inputModeSelector.addItemList(inputModeChoices, 1);
    inputModeSelector.setTooltip("Audio source: external input or internal generator");
    inputModeSelector.setTitle("Input Mode");
    addAndMakeVisible(inputModeSelector);
    inputModeLabel.setText("Input", juce::dontSendNotification);
    inputModeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputModeLabel);

    dopplerDirSelector.addItemList(dopplerDirectionChoices, 1);
    dopplerDirSelector.setTooltip("Direction of the Doppler pitch sweep");
    dopplerDirSelector.setTitle("Doppler Direction");
    addAndMakeVisible(dopplerDirSelector);
    dopplerDirLabel.setText("Direction", juce::dontSendNotification);
    dopplerDirLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dopplerDirLabel);

    syncNoteSelector.addItemList(syncNoteChoices, 1);
    syncNoteSelector.setTooltip("Beat subdivision when synced to host tempo");
    syncNoteSelector.setTitle("Sync Note Value");
    addAndMakeVisible(syncNoteSelector);
    syncNoteLabel.setText("Note", juce::dontSendNotification);
    syncNoteLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(syncNoteLabel);

    // --- Toggles ---
    syncToggle.setButtonText("SYNC");
    syncToggle.setTooltip("Lock transient timing to DAW tempo");
    syncToggle.setTitle("Sync to Host");
    addAndMakeVisible(syncToggle);

    limiterToggle.setButtonText("LIMIT");
    limiterToggle.setTooltip("Brickwall output limiter to prevent clipping");
    limiterToggle.setTitle("Output Limiter");
    addAndMakeVisible(limiterToggle);

    // --- APVTS attachments ---
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

    prevDopplerVisible  = static_cast<int>(shapeParam->load()) == static_cast<int>(EnvelopeShape::Doppler);
    prevSyncOn          = syncParam->load() >= 0.5f;
    prevSineFreqVisible = static_cast<int>(inputModeParam->load()) == ParamDefaults::INPUT_MODE_SINE_INDEX;
    updateConditionalState();
    startTimerHz(STATE_CHECK_HZ);
}

TransientControls::~TransientControls()
{
    stopTimer();
}

void TransientControls::setupSlider(juce::Slider& slider, juce::Label& label,
                                     const juce::String& labelText, double defaultValue)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    slider.setDoubleClickReturnValue(true, defaultValue);
    slider.setTitle(labelText);
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
    const bool isSine    = static_cast<int>(inputModeParam->load()) == ParamDefaults::INPUT_MODE_SINE_INDEX;

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
    auto row1 = bounds.removeFromTop(42);
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

    // Section label + Row 2: Timing
    bounds.removeFromTop(2);
    timingSectionLabel.setBounds(bounds.removeFromTop(12).withWidth(60));
    auto row2 = bounds.removeFromTop(64);
    {
        const int knobWidth = row2.getWidth() / 4;
        placeKnob(row2, knobWidth, attackTimeSlider, attackTimeLabel);
        placeKnob(row2, knobWidth, tailLengthSlider, tailLengthLabel);
        placeKnob(row2, knobWidth, silenceGapSlider, silenceGapLabel);
        placeKnob(row2, knobWidth, preDelaySlider, preDelayLabel);
    }

    // Section label + Row 3: Shape
    bounds.removeFromTop(2);
    shapeSectionLabel.setBounds(bounds.removeFromTop(12).withWidth(60));
    auto row3 = bounds.removeFromTop(64);
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

    // Section label + Row 4: Output
    bounds.removeFromTop(2);
    outputSectionLabel.setBounds(bounds.removeFromTop(12).withWidth(60));
    auto row4 = bounds.removeFromTop(64);
    {
        const int knobWidth = row4.getWidth() / 4;
        placeKnob(row4, knobWidth, hpfSlider, hpfLabel);
        placeKnob(row4, knobWidth, lpfSlider, lpfLabel);
        placeKnob(row4, knobWidth, outputGainSlider, outputGainLabel);
        placeKnob(row4, knobWidth, humanizeSlider, humanizeLabel);
    }

    bounds.removeFromTop(4);

    // Row 5: Toggles
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
