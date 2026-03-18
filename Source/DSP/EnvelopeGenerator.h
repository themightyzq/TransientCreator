#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include "EnvelopeConstants.h"

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
    bool isInTail() const;

    void setTailLength(float ms);
    void setSilenceGap(float ms);
    void setShape(EnvelopeShape shape);

    // Static method for UI visualization — computes envelope amplitude at a normalized position.
    // normalizedPos: 0.0 = start of tail, 1.0 = end of tail
    // referenceTailSamples: number of samples in the tail (used for absolute-time calculations)
    static float computeShapeAtNormalized(float normalizedPos, EnvelopeShape shape, float referenceTailSamples);

private:
    // Internal state machine
    enum class State
    {
        Tail,
        Silence
    };

    void recalculateSampleCounts();
    float computeEnvelopeSample() const;

    // Shape-specific computation (called by computeEnvelopeSample)
    float computeExponential() const;
    float computeLinear() const;
    float computeLogarithmic() const;
    float computeReverseSawtooth() const;
    float computeGaussian() const;
    float computeDoubleTap() const;
    float computePercussive() const;

    // State
    State currentState = State::Silence;
    EnvelopeShape currentShape = EnvelopeShape::Exponential;
    double currentSampleRate = 44100.0;

    // Timing (in ms, set by user)
    float tailLengthMs = 50.0f;
    float silenceGapMs = 100.0f;

    // Timing (in samples, derived from ms + sample rate)
    int tailSamples = 0;
    int gapSamples  = 0;
    int sampleIndex = 0;  // Current position within current state

    // Pre-computed shape coefficients (recalculated when tail length or sample rate changes)
    float expDecayRate       = 0.0f;
    float logDenominator     = 1.0f;  // log(1 + T * k), precomputed
    float gaussianSigma      = 1.0f;
    float doubleTapSpacingSamples = 0.0f;
    float doubleTapDecayRate = 0.0f;
    float percAttackSamples  = 0.0f;
    float percBodySamples    = 0.0f;
    float percDecayRate      = 0.0f;

    // Crossfade state for anti-aliasing at re-trigger boundaries
    bool crossfading = false;
    int crossfadeIndex = 0;
    float previousEndAmplitude = 0.0f;
};
