#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <zqsfx_ui/zqsfx_ui.h>

// ZQ SFX house UI migration (docs/ZQSFX_UI_STYLE_GUIDE.md). TransientLookAndFeel is now a THIN
// SUBCLASS of zqsfx::ui::LookAndFeel: the house LookAndFeel supplies rotary knobs (CC0
// filmstrips, picked by dial size), combo boxes (LCD dropdowns), and slider text-box readouts
// (LCD glass + glow) automatically once drawRotarySlider / drawComboBox / positionComboBoxText /
// drawLabel are left un-overridden here. This subclass keeps only the two overrides the house
// LookAndFeel has no equivalent for: toggle buttons (SYNC/LIMIT) and the vertical fader sliders
// (ATK/HOLD/TAIL), both restyled with house tokens (hard-edged rectangles, no rounded corners,
// per style guide section 6).
//
// The project rule ("all colours come from here", claude.md) is preserved: every static
// constexpr member below is still the single colour source for editor/component code, now
// remapped 1:1 to a house token (see the per-field comment for which token each mirrors).
class TransientLookAndFeel : public zqsfx::ui::LookAndFeel
{
public:
    TransientLookAndFeel();
    ~TransientLookAndFeel() override = default;

    // ---- Toggle buttons (SYNC, LIMIT): the house LookAndFeel has no drawToggleButton override,
    // so this stays -- restyled with house tokens: hard-edged rectangle (no rounded corners),
    // `btn` gradient off-state, `accent` fill + `accentInk` text on-state, matching
    // zqsfx::ui::LookAndFeel::drawButtonBackground's own on/off treatment.
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override;

    // ---- Vertical faders (ATK/HOLD/TAIL): the house LookAndFeel has no drawLinearSlider
    // override, so this stays -- track in `lcdScreenDark` with a `ruleTitle` border, fill in the
    // slider's own section channel colour (trackColourId, set per-fader in setupVerticalFader),
    // hard-edged rectangular thumb -- no rounded pill, no arc/dot decoration.
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // Colour palette -- remapped to house tokens (docs/ZQSFX_UI_STYLE_GUIDE.md section 2).
    static constexpr juce::uint32 BG_DARK       = 0xff0a0b0c;  // == zqsfx::ui::colour::chassisMid
    static constexpr juce::uint32 BG_PANEL      = 0xff121416;  // == zqsfx::ui::colour::panelBot
    static constexpr juce::uint32 KNOB_BG       = 0xff1d2022;  // == zqsfx::ui::colour::panelTop
    static constexpr juce::uint32 KNOB_TRACK    = 0xff3d4448;  // == zqsfx::ui::colour::ledOffRim
    static constexpr juce::uint32 ACCENT        = 0xffe8622a;  // == zqsfx::ui::colour::accent
    static constexpr juce::uint32 ACCENT_BRIGHT = 0xffe8622a;  // == zqsfx::ui::colour::accent (one house accent -- no separate "bright" variant)
    static constexpr juce::uint32 TEXT_PRIMARY  = 0xffc9d4d2;  // == zqsfx::ui::colour::btnText
    static constexpr juce::uint32 TEXT_DIM      = 0xff7a9a94;  // == zqsfx::ui::colour::silkCaption

    // Semantic section colours -> colour-blind-safe channels (style guide section 3:
    // "HyperPrism, Transient Creator: dynamics sky, frequency yellow, modulation purple, output
    // green, timing white"). This product uses 4 of the 5 channels (no modulation section).
    static constexpr juce::uint32 COLOR_SHAPE     = 0xff56b4e9;  // == zqsfx::ui::comp::sky    (dynamics)
    static constexpr juce::uint32 COLOR_FREQUENCY = 0xfff0e442;  // == zqsfx::ui::comp::yellow
    static constexpr juce::uint32 COLOR_TIMING    = 0xffe2e5e8;  // == zqsfx::ui::comp::white
    static constexpr juce::uint32 COLOR_OUTPUT    = 0xff8fe89a;  // == zqsfx::ui::comp::green
};
