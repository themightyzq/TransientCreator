#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

class TransientControls : public juce::Component
{
public:
    TransientControls() = default;
    ~TransientControls() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
};
