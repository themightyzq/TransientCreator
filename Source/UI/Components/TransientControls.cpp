#include "TransientControls.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

TransientControls::TransientControls(juce::AudioProcessorValueTreeState& apvts)
{
    syncParam      = apvts.getRawParameterValue(ParamIDs::SYNC_ENABLED);
    inputModeParam = apvts.getRawParameterValue(ParamIDs::INPUT_MODE);

    // === Vertical faders (purple/timing) ===
    setupVerticalFader(attackTimeFader, attackTimeLabel, "ATK", ParamDefaults::ATTACK_TIME_DEFAULT);
    attackTimeFader.setTooltip("Onset ramp time - 0 ms is instant apex, higher values soften the attack");

    setupVerticalFader(sustainHoldFader, sustainHoldLabel, "HOLD", ParamDefaults::SUSTAIN_HOLD_DEFAULT);
    sustainHoldFader.setTooltip("Hold at peak amplitude before decay begins (% of tail duration)");

    setupVerticalFader(tailLengthFader, tailLengthLabel, "TAIL", ParamDefaults::TAIL_LENGTH_DEFAULT);
    tailLengthFader.setTooltip("Duration of the transient decay in milliseconds");

    // === Primary rotary knobs ===
    setupRotaryKnob(transientGainSlider, transientGainLabel, "Boost", ParamDefaults::TRANSIENT_GAIN_DEFAULT);
    transientGainSlider.setTooltip("Amplify the transient peak - scales with envelope (0 dB = no boost)");
    transientGainSlider.setColour(juce::Slider::rotarySliderFillColourId,
                                   juce::Colour(TransientLookAndFeel::COLOR_SHAPE));

    setupRotaryKnob(pitchStartSlider, pitchStartLabel, "P.Start", ParamDefaults::PITCH_START_DEFAULT);
    pitchStartSlider.setTooltip("Pitch offset at transient start (semitones, + = up, - = down)");
    pitchStartSlider.setColour(juce::Slider::rotarySliderFillColourId,
                                juce::Colour(TransientLookAndFeel::COLOR_SHAPE));

    setupRotaryKnob(pitchEndSlider, pitchEndLabel, "P.End", ParamDefaults::PITCH_END_DEFAULT);
    pitchEndSlider.setTooltip("Pitch offset at transient end (semitones, + = up, - = down)");
    pitchEndSlider.setColour(juce::Slider::rotarySliderFillColourId,
                              juce::Colour(TransientLookAndFeel::COLOR_SHAPE));

    setupRotaryKnob(mixSlider, mixLabel, "Mix", ParamDefaults::MIX_DEFAULT);
    mixSlider.setTooltip("Blend between dry input (0%) and processed transient output (100%)");
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId,
                         juce::Colour(TransientLookAndFeel::COLOR_OUTPUT));

    setupRotaryKnob(outputGainSlider, outputGainLabel, "Gain", ParamDefaults::OUTPUT_GAIN_DEFAULT);
    outputGainSlider.setTooltip("Output level boost or cut in dB");
    outputGainSlider.setColour(juce::Slider::rotarySliderFillColourId,
                                juce::Colour(TransientLookAndFeel::COLOR_OUTPUT));

    // === Secondary rotary knobs ===
    setupRotaryKnob(silenceGapSlider, silenceGapLabel, "Gap", ParamDefaults::SILENCE_GAP_DEFAULT);
    silenceGapSlider.setTooltip("Silence between transients (overridden when Sync is ON)");
    silenceGapSlider.setColour(juce::Slider::rotarySliderFillColourId,
                                juce::Colour(TransientLookAndFeel::COLOR_TIMING));

    setupRotaryKnob(humanizeSlider, humanizeLabel, "Humanize", ParamDefaults::HUMANIZE_DEFAULT);
    humanizeSlider.setTooltip("Per-cycle random variation on timing for organic feel");
    humanizeSlider.setColour(juce::Slider::rotarySliderFillColourId,
                              juce::Colour(TransientLookAndFeel::COLOR_TIMING));

    setupRotaryKnob(sineFreqSlider, sineFreqLabel, "Freq", ParamDefaults::SINE_FREQ_DEFAULT);
    sineFreqSlider.setTooltip("Frequency of the internal sine oscillator");
    sineFreqSlider.setColour(juce::Slider::rotarySliderFillColourId,
                              juce::Colour(TransientLookAndFeel::COLOR_FREQUENCY));

    // === Dropdowns ===
    inputModeSelector.addItemList(inputModeChoices, 1);
    inputModeSelector.setTooltip("Audio source: external input or internal generator");
    inputModeSelector.setTitle("Input Mode");
    addAndMakeVisible(inputModeSelector);
    inputModeLabel.setText("Input", juce::dontSendNotification);
    inputModeLabel.setJustificationType(juce::Justification::centredRight);
    inputModeLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    addAndMakeVisible(inputModeLabel);

    syncNoteSelector.addItemList(syncNoteChoices, 1);
    syncNoteSelector.setTooltip("Beat subdivision when synced to host tempo");
    syncNoteSelector.setTitle("Sync Note Value");
    addAndMakeVisible(syncNoteSelector);
    syncNoteLabel.setText("Note", juce::dontSendNotification);
    syncNoteLabel.setJustificationType(juce::Justification::centredRight);
    syncNoteLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    addAndMakeVisible(syncNoteLabel);

    // === Toggles ===
    syncToggle.setButtonText("SYNC");
    syncToggle.setTooltip("Lock transient timing to DAW tempo");
    syncToggle.setTitle("Sync to Host");
    addAndMakeVisible(syncToggle);

    limiterToggle.setButtonText("LIMIT");
    limiterToggle.setTooltip("Brickwall output limiter to prevent clipping");
    limiterToggle.setTitle("Output Limiter");
    addAndMakeVisible(limiterToggle);

    // === APVTS attachments ===
    attackTimeAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::ATTACK_TIME, attackTimeFader);
    sustainHoldAttachment    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::SUSTAIN_HOLD, sustainHoldFader);
    tailLengthAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::TAIL_LENGTH, tailLengthFader);
    transientGainAttachment  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::TRANSIENT_GAIN, transientGainSlider);
    pitchStartAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::PITCH_START, pitchStartSlider);
    pitchEndAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::PITCH_END, pitchEndSlider);
    mixAttachment            = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::MIX, mixSlider);
    outputGainAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::OUTPUT_GAIN, outputGainSlider);
    silenceGapAttachment     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::SILENCE_GAP, silenceGapSlider);
    humanizeAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::HUMANIZE, humanizeSlider);
    sineFreqAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, ParamIDs::SINE_FREQ, sineFreqSlider);
    inputModeAttachment      = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ParamIDs::INPUT_MODE, inputModeSelector);
    syncNoteAttachment       = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(apvts, ParamIDs::SYNC_NOTE, syncNoteSelector);
    syncAttachment           = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, ParamIDs::SYNC_ENABLED, syncToggle);
    limiterAttachment        = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(apvts, ParamIDs::LIMITER_ON, limiterToggle);

    prevSyncOn          = syncParam->load() >= 0.5f;
    prevSineFreqVisible = static_cast<int>(inputModeParam->load()) == ParamDefaults::INPUT_MODE_SINE_INDEX;
    updateConditionalState();
    startTimerHz(STATE_CHECK_HZ);
}

TransientControls::~TransientControls() { stopTimer(); }

void TransientControls::setupRotaryKnob(juce::Slider& slider, juce::Label& label,
                                         const juce::String& labelText, double defaultValue)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setDoubleClickReturnValue(true, defaultValue);
    slider.setTitle(labelText);
    addAndMakeVisible(slider);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(label);
}

void TransientControls::setupVerticalFader(juce::Slider& slider, juce::Label& label,
                                            const juce::String& labelText, double defaultValue)
{
    slider.setSliderStyle(juce::Slider::LinearVertical);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 64, 16);
    slider.setDoubleClickReturnValue(true, defaultValue);
    slider.setTitle(labelText);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(TransientLookAndFeel::COLOR_TIMING));
    addAndMakeVisible(slider);
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(label);
}

void TransientControls::timerCallback() { updateConditionalState(); }

void TransientControls::updateConditionalState()
{
    const bool isSyncOn = syncParam->load() >= 0.5f;
    const bool isSine = static_cast<int>(inputModeParam->load()) == ParamDefaults::INPUT_MODE_SINE_INDEX;

    sineFreqSlider.setVisible(isSine);
    sineFreqLabel.setVisible(isSine);
    syncNoteSelector.setVisible(isSyncOn);
    syncNoteLabel.setVisible(isSyncOn);
    silenceGapSlider.setEnabled(!isSyncOn);
    silenceGapSlider.setAlpha(isSyncOn ? 0.4f : 1.0f);
    silenceGapLabel.setAlpha(isSyncOn ? 0.4f : 1.0f);

    const bool needsRelayout = (isSyncOn != prevSyncOn) || (isSine != prevSineFreqVisible);
    prevSyncOn = isSyncOn;
    prevSineFreqVisible = isSine;
    if (needsRelayout) resized();
}

void TransientControls::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(TransientLookAndFeel::BG_DARK));

    const int faderWidth = 155;
    const int rightX = faderWidth + 6;
    const int rightW = getWidth() - rightX;
    const int bottomStripH = 34;

    const bool sineVisible = sineFreqSlider.isVisible();
    const int numKnobs = sineVisible ? 8 : 7;
    const int cellW = rightW / std::max(1, numKnobs);

    const int sep1X = rightX + cellW * 3;
    const int sep2X = rightX + cellW * (sineVisible ? 6 : 5);

    // Group labels positioned just above the knob label row
    const int groupLabelY = juce::jmax(0, knobStackTopY - 22);

    g.setFont(juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));

    g.setColour(juce::Colour(TransientLookAndFeel::COLOR_SHAPE).withAlpha(0.45f));
    g.drawText("SHAPE", rightX, groupLabelY, cellW * 3, 20, juce::Justification::centred);

    const int timingCells = sineVisible ? 3 : 2;
    g.setColour(juce::Colour(TransientLookAndFeel::COLOR_TIMING).withAlpha(0.45f));
    g.drawText("TIMING", sep1X, groupLabelY, cellW * timingCells, 20, juce::Justification::centred);

    g.setColour(juce::Colour(TransientLookAndFeel::COLOR_OUTPUT).withAlpha(0.45f));
    g.drawText("OUTPUT", sep2X, groupLabelY, cellW * 2, 20, juce::Justification::centred);

    // Vertical separators spanning from group labels to above bottom strip
    const int sepBottom = getHeight() - bottomStripH - 8;
    g.setColour(juce::Colour(TransientLookAndFeel::KNOB_TRACK).withAlpha(0.5f));
    g.drawLine(static_cast<float>(sep1X), static_cast<float>(groupLabelY),
               static_cast<float>(sep1X), static_cast<float>(sepBottom), 0.5f);
    g.drawLine(static_cast<float>(sep2X), static_cast<float>(groupLabelY),
               static_cast<float>(sep2X), static_cast<float>(sepBottom), 0.5f);

    // Bottom strip separator
    const int knobAreaH = getHeight() - bottomStripH - 4;
    g.setColour(juce::Colour(TransientLookAndFeel::KNOB_TRACK).withAlpha(0.3f));
    g.drawLine(static_cast<float>(rightX), static_cast<float>(knobAreaH),
               static_cast<float>(getWidth() - 4), static_cast<float>(knobAreaH), 0.5f);
}

void TransientControls::resized()
{
    auto bounds = getLocalBounds();

    const int faderColumnWidth = 155;
    auto faderArea = bounds.removeFromLeft(faderColumnWidth);
    bounds.removeFromLeft(6);
    auto rightArea = bounds;

    // === LEFT: Envelope fader group ===
    {
        auto fa = faderArea;
        fa.removeFromTop(18);

        const int faderCount = 3;
        const int faderCellWidth = fa.getWidth() / faderCount;

        for (int i = 0; i < faderCount; ++i)
        {
            auto cell = fa.removeFromLeft(faderCellWidth).reduced(2, 0);
            juce::Slider* slider = nullptr;
            juce::Label* label = nullptr;
            switch (i)
            {
                case 0: slider = &attackTimeFader;  label = &attackTimeLabel;  break;
                case 1: slider = &sustainHoldFader; label = &sustainHoldLabel; break;
                case 2: slider = &tailLengthFader;  label = &tailLengthLabel;  break;
                default: break;
            }
            if (slider == nullptr) continue;
            label->setBounds(cell.removeFromTop(16));
            cell.removeFromTop(2);
            slider->setBounds(cell);
        }
    }

    // === RIGHT: Single knob row + bottom strip ===
    {
        const int bottomStripH = 34;
        auto bottomStrip = rightArea.removeFromBottom(bottomStripH);
        rightArea.removeFromBottom(4);

        auto knobArea = rightArea;

        int numKnobs = 7;
        if (sineFreqSlider.isVisible()) ++numKnobs;
        const int knobCellWidth = knobArea.getWidth() / std::max(1, numKnobs);

        // Centered knob placement: label + knob + value as one tight unit
        auto placeKnob = [](juce::Rectangle<int>& row, int cellW,
                            juce::Slider& sl, juce::Label& lb)
        {
            auto cell = row.removeFromLeft(cellW);

            const int labelH = 16;
            const int gapAfterLabel = 1;
            const int valueH = 20;  // matches setTextBoxStyle height
            const int gapBeforeValue = 1;

            const int knobDiam = juce::jmin(cell.getWidth() - 8,
                                             cell.getHeight() - labelH - valueH - gapAfterLabel - gapBeforeValue - 4);
            const int knobSize = juce::jmax(40, knobDiam);

            const int stackH = labelH + gapAfterLabel + knobSize + gapBeforeValue + valueH;
            const int topY = cell.getY() + (cell.getHeight() - stackH) / 2;

            lb.setBounds(cell.getX(), topY, cell.getWidth(), labelH);

            const int sliderY = topY + labelH + gapAfterLabel;
            const int sliderH = knobSize + gapBeforeValue + valueH;
            sl.setBounds(cell.getX(), sliderY, cell.getWidth(), sliderH);
        };

        placeKnob(knobArea, knobCellWidth, transientGainSlider, transientGainLabel);

        // Capture the Y position of the first label for paint() group labels
        knobStackTopY = transientGainLabel.getY();

        placeKnob(knobArea, knobCellWidth, pitchStartSlider, pitchStartLabel);
        placeKnob(knobArea, knobCellWidth, pitchEndSlider, pitchEndLabel);
        placeKnob(knobArea, knobCellWidth, silenceGapSlider, silenceGapLabel);
        placeKnob(knobArea, knobCellWidth, humanizeSlider, humanizeLabel);
        if (sineFreqSlider.isVisible())
            placeKnob(knobArea, knobCellWidth, sineFreqSlider, sineFreqLabel);
        placeKnob(knobArea, knobCellWidth, mixSlider, mixLabel);
        placeKnob(knobArea, knobCellWidth, outputGainSlider, outputGainLabel);

        // --- Bottom strip ---
        {
            auto inputArea = bottomStrip.removeFromLeft(220);
            inputModeLabel.setBounds(inputArea.removeFromLeft(36).withHeight(26));
            inputArea.removeFromLeft(4);
            inputModeSelector.setBounds(inputArea.withHeight(26));

            bottomStrip.removeFromLeft(12);

            syncToggle.setBounds(bottomStrip.removeFromLeft(60).withHeight(26));

            if (syncNoteSelector.isVisible())
            {
                bottomStrip.removeFromLeft(4);
                syncNoteLabel.setBounds(bottomStrip.removeFromLeft(30).withHeight(26));
                syncNoteSelector.setBounds(bottomStrip.removeFromLeft(70).withHeight(26));
            }

            limiterToggle.setBounds(bottomStrip.removeFromRight(60).withHeight(26));
        }
    }
}
