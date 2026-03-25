#include "MainPanel.h"

MainPanel::MainPanel(juce::AudioProcessorValueTreeState& apvts, SharedUIState& sharedState)
    : apvtsRef(apvts),
      envelopeVisualizer(apvts, sharedState),
      transientControls(apvts)
{
    setLookAndFeel(&lookAndFeel);
    addAndMakeVisible(envelopeVisualizer);
    addAndMakeVisible(transientControls);

    // Shape selector bar
    shapeParam = apvts.getRawParameterValue(ParamIDs::SHAPE);

    shapeBarLabel.setText("SHAPE", juce::dontSendNotification);
    shapeBarLabel.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
    shapeBarLabel.setColour(juce::Label::textColourId,
                             juce::Colour(TransientLookAndFeel::COLOR_SHAPE).withAlpha(0.6f));
    shapeBarLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(shapeBarLabel);

    shapeSelector.addItemList(shapeChoices, 1);
    shapeSelector.setTooltip("Envelope curve shape preset");
    shapeSelector.setTitle("Shape");
    addAndMakeVisible(shapeSelector);

    shapePrevButton.setTooltip("Previous shape");
    shapePrevButton.onClick = [this]()
    {
        if (auto* param = apvtsRef.getParameter(ParamIDs::SHAPE))
        {
            const int current = static_cast<int>(shapeParam->load());
            const int numChoices = shapeChoices.size();
            const int prev = (current - 1 + numChoices) % numChoices;
            param->setValueNotifyingHost(static_cast<float>(prev) / static_cast<float>(numChoices - 1));
        }
    };
    addAndMakeVisible(shapePrevButton);

    shapeNextButton.setTooltip("Next shape");
    shapeNextButton.onClick = [this]()
    {
        if (auto* param = apvtsRef.getParameter(ParamIDs::SHAPE))
        {
            const int current = static_cast<int>(shapeParam->load());
            const int numChoices = shapeChoices.size();
            const int next = (current + 1) % numChoices;
            param->setValueNotifyingHost(static_cast<float>(next) / static_cast<float>(numChoices - 1));
        }
    };
    addAndMakeVisible(shapeNextButton);

    shapeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, ParamIDs::SHAPE, shapeSelector);
}

MainPanel::~MainPanel()
{
    setLookAndFeel(nullptr);
}

void MainPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(TransientLookAndFeel::BG_DARK));

    auto bounds = getLocalBounds();

    // --- Header (48px) ---
    auto header = bounds.removeFromTop(48);
    g.setColour(juce::Colour(TransientLookAndFeel::ACCENT).withAlpha(0.4f));
    g.fillRect(12, 3, getWidth() - 24, 2);
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_PRIMARY));
    g.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
    g.drawText("Transient Creator", header.reduced(12, 0).withTrimmedTop(10).withHeight(20),
               juce::Justification::centred);
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("ZQSFX", header.reduced(12, 0).withTrimmedTop(30).withHeight(14),
               juce::Justification::centred);

    // --- Fader group background ---
    auto contentCalc = getLocalBounds();
    contentCalc.removeFromTop(48);
    contentCalc.removeFromBottom(16);
    auto contentR = contentCalc.reduced(8, 4);
    auto calcVizH = static_cast<int>(contentR.getHeight() * 0.35f);
    int controlsTop = contentR.getY() + calcVizH + 28 + 6;  // viz + shape bar + gap

    auto faderBg = juce::Rectangle<int>(8, controlsTop, 155,
                                         getHeight() - controlsTop - 16 - 4);
    g.setColour(juce::Colour(TransientLookAndFeel::BG_PANEL));
    g.fillRoundedRectangle(faderBg.toFloat(), 6.0f);

    // ENVELOPE label at top of fader panel
    g.setColour(juce::Colour(TransientLookAndFeel::COLOR_TIMING).withAlpha(0.6f));
    g.setFont(juce::Font(juce::FontOptions(8.0f).withStyle("Bold")));
    g.drawText("ENVELOPE", faderBg.removeFromTop(16), juce::Justification::centred);

    // Thin underline
    g.setColour(juce::Colour(TransientLookAndFeel::COLOR_TIMING).withAlpha(0.2f));
    g.drawLine(static_cast<float>(faderBg.getX() + 8), static_cast<float>(faderBg.getY()),
               static_cast<float>(faderBg.getRight() - 8), static_cast<float>(faderBg.getY()), 0.5f);

    // --- Footer ---
    auto footer = getLocalBounds().removeFromBottom(16);
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.5f));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("v1.0.0", footer.removeFromRight(70), juce::Justification::centredRight);
}

void MainPanel::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(48);
    bounds.removeFromBottom(16);
    auto content = bounds.reduced(8, 4);

    // Curve editor (35%)
    auto vizHeight = static_cast<int>(content.getHeight() * 0.35f);
    envelopeVisualizer.setBounds(content.removeFromTop(vizHeight));

    // Shape bar (28px, centered)
    auto shapeBar = content.removeFromTop(28);
    {
        const int labelW = 42;
        const int btnW = 24;
        const int dropdownW = 160;
        const int gaps = 6;
        const int totalW = labelW + btnW + dropdownW + btnW + gaps;
        const int startX = (shapeBar.getWidth() - totalW) / 2;

        auto bar = shapeBar.withX(shapeBar.getX() + startX).withWidth(totalW);

        shapeBarLabel.setBounds(bar.removeFromLeft(labelW).withHeight(28));
        bar.removeFromLeft(2);
        shapePrevButton.setBounds(bar.removeFromLeft(btnW).reduced(0, 3));
        bar.removeFromLeft(2);
        shapeSelector.setBounds(bar.removeFromLeft(dropdownW).reduced(0, 3));
        bar.removeFromLeft(2);
        shapeNextButton.setBounds(bar.removeFromLeft(btnW).reduced(0, 3));
    }

    content.removeFromTop(6);

    // Controls
    transientControls.setBounds(content);
}
