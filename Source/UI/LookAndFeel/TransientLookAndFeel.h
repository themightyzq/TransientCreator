#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class TransientLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TransientLookAndFeel() = default;
    ~TransientLookAndFeel() override = default;
};
