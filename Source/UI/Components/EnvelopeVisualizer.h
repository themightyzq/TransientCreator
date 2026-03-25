#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../DSP/EnvelopeGenerator.h"
#include "../../Parameters/ParameterLayout.h"
#include "../../SharedState.h"

class EnvelopeVisualizer : public juce::Component, private juce::Timer,
                            private juce::AudioProcessorValueTreeState::Listener
{
public:
    EnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts, SharedUIState& sharedState);
    ~EnvelopeVisualizer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    void loadShapeIntoBreakpoints(EnvelopeShape shape);
    void rebuildAndPush();
    void updateLayoutCache();

    struct DecayCoord { float normX; float normY; bool inDecayRegion; };
    DecayCoord pixelToDecayCoord(juce::Point<float> pixel) const;
    juce::Point<float> decayCoordToPixel(float decayNormX, float decayNormY) const;
    int findNearestBreakpoint(juce::Point<float> pixel, float maxDistPx) const;

    static constexpr int REPAINT_HZ          = 30;
    static constexpr int RESOLUTION          = 256;
    static constexpr float TAIL_FRACTION     = 0.65f;
    static constexpr float REFERENCE_RATE    = 44100.0f;
    static constexpr float POINT_RADIUS      = 4.0f;
    static constexpr float POINT_HIT_RADIUS  = 10.0f;

    juce::AudioProcessorValueTreeState& apvtsRef;
    SharedUIState& sharedStateRef;

    std::atomic<float>* shapeParam      = nullptr;
    std::atomic<float>* tailLengthParam = nullptr;
    std::atomic<float>* silenceGapParam = nullptr;
    std::atomic<float>* attackTimeParam = nullptr;
    std::atomic<float>* holdParam       = nullptr;

    std::array<float, SharedUIState::CUSTOM_CURVE_SIZE> localCurve {};

    // Breakpoint interaction
    int dragIndex = -1;
    int hoveredIndex = -1;
    bool mouseInside = false;
    juce::Point<float> hoverPixel {};
    bool curveIsModified = false;

    // Tension drag state
    int tensionDragSegment = -1;
    float tensionDragStartY = 0.0f;
    float tensionDragStartVal = 0.0f;

    // Cached layout
    juce::Rectangle<float> cachedBounds {};
    float cachedTailFraction = 0.65f;
    float cachedDecayStart = 0.0f;
};
