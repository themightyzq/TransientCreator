#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Components/EnvelopeVisualizer.h"
#include "../Components/TransientControls.h"

class MainPanel : public juce::Component
{
public:
    MainPanel();
    ~MainPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    EnvelopeVisualizer envelopeVisualizer;
    TransientControls transientControls;
};
