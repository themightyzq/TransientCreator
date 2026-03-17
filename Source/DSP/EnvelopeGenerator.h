#pragma once

#include <juce_core/juce_core.h>

enum class EnvelopeShape
{
    Exponential = 0,
    Linear,
    Logarithmic,
    Doppler,
    ReverseSawtooth,
    Gaussian,
    DoubleTap,
    Percussive
};

class EnvelopeGenerator
{
public:
    EnvelopeGenerator() = default;
    ~EnvelopeGenerator() = default;

    void prepare(double sampleRate);
    void trigger();
    float getNextSample();
    bool isActive() const;

    void setTailLength(float ms);
    void setSilenceGap(float ms);
    void setShape(EnvelopeShape shape);

private:
    double currentSampleRate = 44100.0;
    EnvelopeShape currentShape = EnvelopeShape::Exponential;
    float tailLengthMs = 50.0f;
    float silenceGapMs = 100.0f;
    bool active = false;
};
