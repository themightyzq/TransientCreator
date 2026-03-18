#pragma once

#include <cmath>
#include <algorithm>
#include <array>

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

    // DSP constants
    static constexpr float ENVELOPE_THRESHOLD  = 0.001f;    // -60dB floor
    static constexpr float LOG_CURVATURE_K     = 10.0f;     // Logarithmic shape curvature
    static constexpr float DOUBLE_TAP_SPACING  = 0.3f;      // Second tap at 30% of tail
    static constexpr float PERCUSSIVE_ATTACK_S = 0.001f;     // 1ms attack
    static constexpr float PERCUSSIVE_BODY_RATIO = 0.15f;   // 15% of tail is body
    static constexpr float PERCUSSIVE_BODY_DROP  = 0.3f;    // Body drops to 0.7 amplitude
    static constexpr float GAUSSIAN_SIGMA_RATIO  = 0.25f;   // sigma = T / 4
    static constexpr int   CROSSFADE_SAMPLES     = 32;      // Anti-click crossfade length
    static constexpr float FADE_IN_MS            = 1.0f;    // Onset fade-in to prevent click

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

    // Onset fade-in (applied at the very start of each transient)
    int fadeInSamples = 0;

    // Crossfade state for anti-aliasing at re-trigger boundaries
    bool crossfading = false;
    int crossfadeIndex = 0;
    float previousEndAmplitude = 0.0f;
};
