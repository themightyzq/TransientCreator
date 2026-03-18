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
    bool isInTail() const;

    void setTailLength(float ms);
    void setSilenceGap(float ms);
    void setShape(EnvelopeShape shape);

    // Returns true once after trigger() is called, then resets.
    // Used by TransientEngine to sync Doppler sweep to envelope re-trigger.
    bool consumeTriggerFlag()
    {
        bool t = justTriggered;
        justTriggered = false;
        return t;
    }

    // Static method for UI visualization — computes envelope amplitude at a normalized position.
    static float computeShapeAtNormalized(float normalizedPos, EnvelopeShape shape, float referenceTailSamples);

private:
    enum class State
    {
        Tail,
        Silence
    };

    void recalculateSampleCounts();
    float computeEnvelopeSample() const;

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

    // Parameter caching — skip recalculation when values haven't changed
    float cachedTailLengthMs = -1.0f;
    float cachedSilenceGapMs = -1.0f;

    // Timing (in samples, derived from ms + sample rate)
    int tailSamples = 0;
    int gapSamples  = 0;
    int sampleIndex = 0;

    // Pre-computed shape coefficients
    float expDecayRate       = 0.0f;
    float logDenominator     = 1.0f;
    float gaussianSigma      = 1.0f;
    float doubleTapSpacingSamples = 0.0f;
    float doubleTapDecayRate = 0.0f;
    float percAttackSamples  = 0.0f;
    float percBodySamples    = 0.0f;
    float percDecayRate      = 0.0f;

    // Trigger flag for Doppler sync
    bool justTriggered = false;

    // Crossfade state for anti-aliasing at re-trigger boundaries
    bool crossfading = false;
    int crossfadeIndex = 0;
    float previousEndAmplitude = 0.0f;
};
