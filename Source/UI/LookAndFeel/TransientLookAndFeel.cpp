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
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

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
    const float trackWidth = 3.0f;
    juce::Path trackArc;
    trackArc.addCentredArc(centreX, centreY, radius - trackWidth, radius - trackWidth,
                           0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(juce::Colour(KNOB_TRACK));
    g.strokePath(trackArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    // Value arc
    if (sliderPos > 0.0f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc(centreX, centreY, radius - trackWidth, radius - trackWidth,
                               0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(ACCENT));
        g.strokePath(valueArc, juce::PathStrokeType(trackWidth, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // Pointer line
    const float pointerLength = radius * 0.6f;
    const float pointerThickness = 2.5f;
    juce::Path pointer;
    pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(juce::Colour(ACCENT_BRIGHT));
    g.fillPath(pointer);

    // Focus ring — keyboard accessibility indicator
    if (slider.hasKeyboardFocus(true))
    {
        g.setColour(juce::Colour(ACCENT_BRIGHT).withAlpha(0.4f));
        g.drawEllipse(centreX - radius - 2.0f, centreY - radius - 2.0f,
                      (radius + 2.0f) * 2.0f, (radius + 2.0f) * 2.0f, 1.5f);
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
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(bounds, cornerSize);
    }

    g.setColour(button.getToggleState() ? juce::Colours::white : juce::Colour(TEXT_DIM));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}
