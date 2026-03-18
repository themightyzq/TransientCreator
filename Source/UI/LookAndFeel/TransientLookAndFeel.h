#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class TransientLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TransientLookAndFeel();
    ~TransientLookAndFeel() override = default;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // Colour palette
    static constexpr juce::uint32 BG_DARK      = 0xff0a0e1a;
    static constexpr juce::uint32 BG_PANEL      = 0xff111827;
    static constexpr juce::uint32 ACCENT        = 0xff3b82f6;
    static constexpr juce::uint32 ACCENT_BRIGHT = 0xff60a5fa;
    static constexpr juce::uint32 KNOB_BG       = 0xff1e293b;
    static constexpr juce::uint32 KNOB_TRACK    = 0xff334155;
    static constexpr juce::uint32 TEXT_PRIMARY   = 0xffe2e8f0;
    static constexpr juce::uint32 TEXT_DIM       = 0xff94a3b8;
};
