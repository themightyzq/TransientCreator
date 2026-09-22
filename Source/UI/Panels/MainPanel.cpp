#include "MainPanel.h"

MainPanel::MainPanel(juce::AudioProcessorValueTreeState& apvts, SharedUIState& sharedState)
    : apvtsRef(apvts),
      envelopeVisualizer(apvts, sharedState),
      transientControls(apvts)
{
    addAndMakeVisible(envelopeVisualizer);
    addAndMakeVisible(transientControls);

    // The ZQ SFX mark (style guide section 5): header row, far right; also the About-box
    // trigger (LogoMark sets its own tooltip/title/description to "About Transient Creator"
    // already).
    logo.onClick = [this] { showAboutBox(); };
    addAndMakeVisible(logo);

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
    shapeSelector.setDescription(shapeSelector.getTooltip());
    addAndMakeVisible(shapeSelector);

    shapePrevButton.setTooltip("Previous shape");
    shapePrevButton.setTitle("Previous shape");
    shapePrevButton.setDescription(shapePrevButton.getTooltip());
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
    shapeNextButton.setTitle("Next shape");
    shapeNextButton.setDescription(shapeNextButton.getTooltip());
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

MainPanel::~MainPanel() = default;

void MainPanel::showAboutBox()
{
    // ASCII-only (style guide section 5 / migration spec); product name + version from the real
    // build (JucePlugin_VersionString, generated from CMakeLists.txt's project(... VERSION ...)),
    // not a hand-maintained literal that could drift from it.
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon, "About Transient Creator",
        juce::String("Transient Creator ") + JucePlugin_VersionString +
            "\n\nZQ SFX - https://www.zq-sfx.com - connect@zq-sfx.com\n"
            "Free software under GPL-3.0-or-later. Built with JUCE.\n"
            "Fonts: Barlow Condensed, VT323, IBM Plex Mono (SIL OFL).\n"
            "Knobs: CC0 designs from the g200kg KnobGallery.",
        "Close", this);
}

void MainPanel::paint(juce::Graphics& g)
{
    // Window background = house chassis gradient (style guide section 2 / migration spec) in
    // place of the old flat BG_DARK fill.
    g.setGradientFill(zqsfx::ui::gradients::chassis(getLocalBounds().toFloat()));
    g.fillAll();

    auto bounds = getLocalBounds();

    // --- Header (48px) ---
    auto header = bounds.removeFromTop(48);
    g.setColour(juce::Colour(TransientLookAndFeel::ACCENT).withAlpha(0.4f));
    g.fillRect(12, 3, getWidth() - 24, 2);
    // Product wordmark: stays with the product (style guide section 1), not routed through the
    // house LookAndFeel's fonts. Vertically re-centred in the header now that the old "ZQ SFX"
    // caption line beneath it is gone (replaced by the LogoMark at the header's far right).
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_PRIMARY));
    g.setFont(juce::Font(juce::FontOptions(15.0f).withStyle("Bold")));
    g.drawText("Transient Creator", header.reduced(12, 0).withTrimmedTop(14).withHeight(20),
               juce::Justification::centred);

    // --- Footer ---
    auto footer = getLocalBounds().removeFromBottom(16);
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.5f));
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("v1.0.0", footer.removeFromRight(70), juce::Justification::centredRight);
}

void MainPanel::resized()
{
    auto bounds = getLocalBounds();
    auto header = bounds.removeFromTop(48);

    // ZQ SFX mark (style guide section 5): header row, far right, at least 24 px tall. Reserved
    // first so it is always the rightmost element regardless of window width; the wordmark's
    // painted text (paint()) is centred across the whole header and has ample room to spare even
    // at the 600 px minimum width.
    logo.setBounds(header.reduced(12, 0).removeFromRight(28).withSizeKeepingCentre(28, 28));

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
