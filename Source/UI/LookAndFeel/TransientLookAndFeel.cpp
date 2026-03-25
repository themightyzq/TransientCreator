#include "TransientLookAndFeel.h"

TransientLookAndFeel::TransientLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(TEXT_PRIMARY));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(KNOB_BG));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    setColour(juce::ComboBox::backgroundColourId, juce::Colour(KNOB_BG));
    setColour(juce::ComboBox::textColourId, juce::Colour(TEXT_PRIMARY));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(KNOB_TRACK));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(TEXT_DIM));

    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(BG_PANEL));
    setColour(juce::PopupMenu::textColourId, juce::Colour(TEXT_PRIMARY));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(ACCENT));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(TEXT_PRIMARY));

    setColour(juce::Label::textColourId, juce::Colour(TEXT_DIM));
}

void TransientLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                             juce::Slider& slider)
{
    const float radius = static_cast<float>(juce::jmin(width, height)) * 0.4f;
    const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float centreY = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Background circle
    g.setColour(juce::Colour(KNOB_BG));
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    // Track arc
    const float trackWidth = 3.5f;
    juce::Path trackArc;
    trackArc.addCentredArc(centreX, centreY, radius - trackWidth, radius - trackWidth,
                           0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(KNOB_TRACK));
    g.strokePath(trackArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Midpoint reference dot on the arc track
    {
        const float midAngle = (rotaryStartAngle + rotaryEndAngle) * 0.5f;
        const float dotRadius = 2.5f;
        const float dotDist = radius - trackWidth;
        const float dotX = centreX + dotDist * std::sin(midAngle);
        const float dotY = centreY - dotDist * std::cos(midAngle);
        g.setColour(juce::Colour(KNOB_TRACK).brighter(0.3f));
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2.0f, dotRadius * 2.0f);
    }

    // Determine knob color — use per-slider override if set, otherwise fall back to ACCENT
    const bool hasCustomColour = slider.isColourSpecified(juce::Slider::rotarySliderFillColourId);
    const juce::Colour arcColour = hasCustomColour
        ? slider.findColour(juce::Slider::rotarySliderFillColourId)
        : juce::Colour(ACCENT);
    const juce::Colour pointerColour = hasCustomColour
        ? arcColour.brighter(0.3f)
        : juce::Colour(ACCENT_BRIGHT);

    // Value arc
    if (sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius - trackWidth, radius - trackWidth,
                               0.0f, rotaryStartAngle, angle, true);
        g.setColour(arcColour);
        g.strokePath(valueArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // Pointer line
    const float pointerLength = radius * 0.6f;
    const float pointerThickness = 3.0f;
    juce::Path pointer;
    pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(pointerColour);
    g.fillPath(pointer);

    // Focus ring — keyboard accessibility indicator
    if (slider.hasKeyboardFocus(true))
    {
        g.setColour(arcColour.withAlpha(0.4f));
        g.drawEllipse(centreX - radius - 2.0f, centreY - radius - 2.0f,
                      (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 1.5f);
    }
}

void TransientLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                             juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (style != juce::Slider::LinearVertical)
    {
        LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, 0, 0, style, slider);
        return;
    }

    const bool hasCustomColour = slider.isColourSpecified(juce::Slider::trackColourId);
    const juce::Colour fillColour = hasCustomColour
        ? slider.findColour(juce::Slider::trackColourId)
        : juce::Colour(ACCENT);

    const float trackWidth = 5.0f;
    const float centreX = static_cast<float>(x) + static_cast<float>(width) * 0.5f;
    const float topY = static_cast<float>(y);
    const float bottomY = static_cast<float>(y + height);

    // Track background
    g.setColour(juce::Colour(KNOB_TRACK));
    g.fillRoundedRectangle(centreX - trackWidth * 0.5f, topY, trackWidth, bottomY - topY, trackWidth * 0.5f);

    // Track tick marks at 25%, 50%, 75%
    g.setColour(juce::Colour(KNOB_TRACK).brighter(0.2f));
    for (float pct : { 0.25f, 0.5f, 0.75f })
    {
        const float tickY = bottomY - pct * (bottomY - topY);
        g.drawLine(centreX - 8.0f, tickY, centreX + 8.0f, tickY, 0.5f);
    }

    // Filled portion (from bottom up to thumb)
    const float fillHeight = bottomY - sliderPos;
    if (fillHeight > 0.0f)
    {
        g.setColour(fillColour);
        g.fillRoundedRectangle(centreX - trackWidth * 0.5f, sliderPos, trackWidth, fillHeight, trackWidth * 0.5f);
    }

    // Thumb — wider with center groove
    const float thumbWidth = 30.0f;
    const float thumbHeight = 12.0f;
    const float thumbX = centreX - thumbWidth * 0.5f;
    const float thumbY = sliderPos - thumbHeight * 0.5f;

    g.setColour(fillColour.brighter(0.25f));
    g.fillRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f);

    // Center groove line
    g.setColour(fillColour.darker(0.2f));
    g.drawLine(thumbX + 6.0f, sliderPos, thumbX + thumbWidth - 6.0f, sliderPos, 1.0f);

    // Subtle border
    g.setColour(fillColour.withAlpha(0.3f));
    g.drawRoundedRectangle(thumbX, thumbY, thumbWidth, thumbHeight, 3.0f, 0.5f);

    // Focus ring
    if (slider.hasKeyboardFocus(true))
    {
        g.setColour(fillColour.withAlpha(0.4f));
        g.drawRoundedRectangle(thumbX - 2.0f, thumbY - 2.0f,
                               thumbWidth + 4.0f, thumbHeight + 4.0f, 5.0f, 1.5f);
    }
}

void TransientLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool /*shouldDrawButtonAsDown*/)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced(2.0f);
    const float cornerSize = 4.0f;

    if (button.getToggleState())
    {
        g.setColour(juce::Colour(ACCENT));
        g.fillRoundedRectangle(bounds, cornerSize);
    }
    else
    {
        g.setColour(juce::Colour(KNOB_BG));
        g.fillRoundedRectangle(bounds, cornerSize);
        g.setColour(juce::Colour(KNOB_TRACK));
        g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
    }

    if (shouldDrawButtonAsHighlighted)
    {
        g.setColour(juce::Colour(TEXT_PRIMARY).withAlpha(0.1f));
        g.fillRoundedRectangle(bounds, cornerSize);
    }

    g.setColour(button.getToggleState() ? juce::Colour(TEXT_PRIMARY) : juce::Colour(TEXT_DIM));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}
