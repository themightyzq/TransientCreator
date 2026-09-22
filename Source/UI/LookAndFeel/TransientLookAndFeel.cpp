#include "TransientLookAndFeel.h"

TransientLookAndFeel::TransientLookAndFeel()
{
    // The base zqsfx::ui::LookAndFeel constructor already sets the house colours this class used
    // to set itself: ComboBox/PopupMenu -> LCD glass, Slider textbox -> LCD glass + glow,
    // TextButton -> btn gradient / accent-on, Label -> silkLabel, TooltipWindow/AlertWindow/
    // TextEditor -> house tokens. Nothing here needs to re-set or override any of that.
    // rotarySliderFillColourId is gone too: the house's filmstrip knobs carry their own pointer
    // and consult no per-slider colour at all (see zqsfx::ui::LookAndFeel::drawRotarySlider /
    // drawVectorKnob).
}

// ---------------------------------------------------------------------- Toggle buttons

void TransientLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                             bool /*shouldDrawButtonAsHighlighted*/,
                                             bool /*shouldDrawButtonAsDown*/)
{
    namespace colour = zqsfx::ui::colour;

    const auto bounds = button.getLocalBounds().toFloat();
    const bool on = button.getToggleState();

    // Hard-edged rectangle -- no rounded pill, no corner radius (style guide section 6).
    if (on)
    {
        g.setColour(colour::accent);
        g.fillRect(bounds);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRect(bounds.withTop(bounds.getBottom() - 2.0f));
    }
    else
    {
        g.setGradientFill(zqsfx::ui::gradients::button(bounds, button.isEnabled()));
        g.fillRect(bounds);
        g.setColour(juce::Colours::white.withAlpha(button.isEnabled() ? 0.07f : 0.0f));
        g.fillRect(bounds.withHeight(1.0f));
    }
    g.setColour(colour::btnBorder);
    g.drawRect(bounds, 1.0f);

    const auto textColour = ! button.isEnabled() ? colour::silkCaption
                           : on                   ? colour::accentInk
                                                   : juce::Colour(TEXT_PRIMARY);
    g.setColour(textColour);
    g.setFont(silkFont(13.0f, true));
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

// ---------------------------------------------------------------------- Linear sliders (ATK/HOLD/TAIL)

void TransientLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                             juce::Slider::SliderStyle style, juce::Slider& slider)
{
    namespace colour = zqsfx::ui::colour;

    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
        return;
    }

    const bool hasCustomColour = slider.isColourSpecified(juce::Slider::trackColourId);
    const juce::Colour fillColour = hasCustomColour
        ? slider.findColour(juce::Slider::trackColourId)
        : colour::accent;

    const float trackWidth = 6.0f;
    const float centreX = (float) x + (float) width * 0.5f;
    const float topY = (float) y;
    const float bottomY = (float) (y + height);
    const juce::Rectangle<float> track(centreX - trackWidth * 0.5f, topY, trackWidth, bottomY - topY);

    // Screen-glass track, ruleTitle border -- hard rectangle, no rounded caps (style guide
    // section 6).
    g.setColour(colour::lcdScreenDark);
    g.fillRect(track);
    g.setColour(colour::ruleTitle);
    g.drawRect(track, 1.0f);

    // Track tick marks at 25%, 50%, 75%.
    g.setColour(colour::ruleTitle.brighter(0.5f));
    for (float pct : { 0.25f, 0.5f, 0.75f })
    {
        const float tickY = bottomY - pct * (bottomY - topY);
        g.drawLine(centreX - 8.0f, tickY, centreX + 8.0f, tickY, 0.5f);
    }

    // Filled portion (from bottom up to thumb), in the control's own section channel colour.
    const float fillHeight = bottomY - sliderPos;
    if (fillHeight > 0.0f)
    {
        g.setColour(fillColour);
        g.fillRect(juce::Rectangle<float>(centreX - trackWidth * 0.5f, sliderPos, trackWidth, fillHeight));
    }

    // Thumb -- hard-edged rectangle, no rounded corners.
    const float thumbWidth = 30.0f;
    const float thumbHeight = 12.0f;
    const float thumbX = centreX - thumbWidth * 0.5f;
    const float thumbY = sliderPos - thumbHeight * 0.5f;
    const juce::Rectangle<float> thumb(thumbX, thumbY, thumbWidth, thumbHeight);

    g.setColour(fillColour.brighter(0.25f));
    g.fillRect(thumb);
    g.setColour(colour::pointer);
    g.drawLine(thumbX + 6.0f, sliderPos, thumbX + thumbWidth - 6.0f, sliderPos, 1.0f);
    g.setColour(colour::panelBorder);
    g.drawRect(thumb, 1.0f);

    // Focus ring comes from the house LookAndFeel (createFocusOutlineForComponent) -- no
    // hand-drawn focus ring here any more (style guide section 8 / accessibility floor item 2).
}
