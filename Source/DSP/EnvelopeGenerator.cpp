#include "EnvelopeGenerator.h"

using namespace EnvelopeConstants;

void EnvelopeGenerator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;

    // Force recalculation on first block after prepare
    cachedTailLengthMs = -1.0f;
    cachedSilenceGapMs = -1.0f;
    recalculateSampleCounts();

    // Reset state
    currentState = State::Silence;
    sampleIndex = 0;
    justTriggered = false;
    crossfading = false;
    crossfadeIndex = 0;
    previousEndAmplitude = 0.0f;
}

void EnvelopeGenerator::trigger()
{
    // Capture the amplitude at the end of the previous state for crossfading
    if (currentState == State::Silence)
        previousEndAmplitude = 0.0f;
    else
        previousEndAmplitude = computeEnvelopeSample();

    currentState = State::Tail;
    sampleIndex = 0;
    justTriggered = true;

    // Enable crossfade if previous amplitude is non-negligible
    // Percussive shape starts from zero (attack phase), so crossfade is unnecessary
    if (previousEndAmplitude > ENVELOPE_THRESHOLD && currentShape != EnvelopeShape::Percussive)
    {
        crossfading = true;
        crossfadeIndex = 0;
    }
    else
    {
        crossfading = false;
    }
}

float EnvelopeGenerator::getNextSample()
{
    float output = 0.0f;

    if (currentState == State::Tail)
    {
        if (sampleIndex >= tailSamples)
        {
            currentState = State::Silence;
            sampleIndex = 0;
            return 0.0f;
        }

        output = computeEnvelopeSample();

        // Apply crossfade at re-trigger boundary
        if (crossfading)
        {
            const float fadeFactor = static_cast<float>(crossfadeIndex) / static_cast<float>(CROSSFADE_SAMPLES);
            output = previousEndAmplitude * (1.0f - fadeFactor) + output * fadeFactor;

            ++crossfadeIndex;
            if (crossfadeIndex >= CROSSFADE_SAMPLES)
                crossfading = false;
        }

        ++sampleIndex;
    }
    else // State::Silence
    {
        if (sampleIndex >= gapSamples)
        {
            trigger();

            output = computeEnvelopeSample();

            if (crossfading)
            {
                const float fadeFactor = static_cast<float>(crossfadeIndex) / static_cast<float>(CROSSFADE_SAMPLES);
                output = previousEndAmplitude * (1.0f - fadeFactor) + output * fadeFactor;
                ++crossfadeIndex;
                if (crossfadeIndex >= CROSSFADE_SAMPLES)
                    crossfading = false;
            }

            ++sampleIndex;
        }
        else
        {
            ++sampleIndex;
            output = 0.0f;
        }
    }

    return output;
}

bool EnvelopeGenerator::isInTail() const
{
    return currentState == State::Tail;
}

void EnvelopeGenerator::setTailLength(float ms)
{
    if (std::fabs(ms - cachedTailLengthMs) < 1.0e-6f) return;
    tailLengthMs = ms;
    cachedTailLengthMs = ms;
    recalculateSampleCounts();
}

void EnvelopeGenerator::setSilenceGap(float ms)
{
    if (std::fabs(ms - cachedSilenceGapMs) < 1.0e-6f) return;
    silenceGapMs = ms;
    cachedSilenceGapMs = ms;
    recalculateSampleCounts();
}

void EnvelopeGenerator::setShape(EnvelopeShape shape)
{
    currentShape = shape;
}

// ---------------------------------------------------------------------------
// Static visualization method — uses same math as DSP, shared constants
// ---------------------------------------------------------------------------

float EnvelopeGenerator::computeShapeAtNormalized(float normalizedPos, EnvelopeShape shape, float referenceTailSamples)
{
    const float t = normalizedPos * referenceTailSamples;
    const float T = referenceTailSamples;

    switch (shape)
    {
        case EnvelopeShape::Exponential:
        case EnvelopeShape::Doppler:
        {
            const float decayRate = -std::log(ENVELOPE_THRESHOLD) / T;
            return std::exp(-t * decayRate);
        }

        case EnvelopeShape::Linear:
            return 1.0f - (t / T);

        case EnvelopeShape::Logarithmic:
        {
            const float denom = std::log(1.0f + T * LOG_CURVATURE_K);
            return 1.0f - std::log(1.0f + t * LOG_CURVATURE_K) / denom;
        }

        case EnvelopeShape::ReverseSawtooth:
            return (t < T) ? (1.0f - t / T) : 0.0f;

        case EnvelopeShape::Gaussian:
        {
            const float sigma = T * GAUSSIAN_SIGMA_RATIO;
            const float ratio = t / sigma;
            return std::exp(-0.5f * ratio * ratio);
        }

        case EnvelopeShape::DoubleTap:
        {
            const float decayRate = -std::log(ENVELOPE_THRESHOLD) / (T * 0.5f);
            const float tap1 = std::exp(-t * decayRate);
            float tap2 = 0.0f;
            const float spacing = T * DOUBLE_TAP_SPACING;
            if (t >= spacing)
                tap2 = std::exp(-(t - spacing) * decayRate);
            return std::max(tap1, tap2);
        }

        case EnvelopeShape::Percussive:
        {
            // Caller passes referenceTailSamples = tailMs/1000 * 44100 (reference rate).
            // Attack in samples at that rate: PERCUSSIVE_ATTACK_S * 44100.
            static constexpr float REFERENCE_RATE = 44100.0f;
            const float percAttack = PERCUSSIVE_ATTACK_S * REFERENCE_RATE;
            const float percBody = T * PERCUSSIVE_BODY_RATIO;

            if (t < percAttack)
                return (percAttack > 0.0f) ? (t / percAttack) : 1.0f;

            const float tAfterAttack = t - percAttack;
            if (tAfterAttack < percBody)
                return (percBody > 0.0f) ? (1.0f - PERCUSSIVE_BODY_DROP * (tAfterAttack / percBody)) : (1.0f - PERCUSSIVE_BODY_DROP);

            const float tAfterBody = tAfterAttack - percBody;
            const float decayLen = T - percAttack - percBody;
            const float decayRate = (decayLen > 0.0f)
                ? -std::log(ENVELOPE_THRESHOLD / (1.0f - PERCUSSIVE_BODY_DROP)) / decayLen
                : 0.0f;
            return (1.0f - PERCUSSIVE_BODY_DROP) * std::exp(-tAfterBody * decayRate);
        }

        default:
            return 0.0f;
    }
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void EnvelopeGenerator::recalculateSampleCounts()
{
    tailSamples = std::max(1, static_cast<int>((tailLengthMs / 1000.0f) * currentSampleRate));
    gapSamples  = std::max(0, static_cast<int>((silenceGapMs / 1000.0f) * currentSampleRate));

    const float T = static_cast<float>(tailSamples);

    expDecayRate = -std::log(ENVELOPE_THRESHOLD) / T;
    logDenominator = std::log(1.0f + T * LOG_CURVATURE_K);
    gaussianSigma = T * GAUSSIAN_SIGMA_RATIO;

    doubleTapSpacingSamples = T * DOUBLE_TAP_SPACING;
    doubleTapDecayRate = -std::log(ENVELOPE_THRESHOLD) / (T * 0.5f);

    percAttackSamples = static_cast<float>(PERCUSSIVE_ATTACK_S * currentSampleRate);
    percBodySamples   = T * PERCUSSIVE_BODY_RATIO;
    const float decayLength = T - percAttackSamples - percBodySamples;
    if (decayLength > 0.0f)
        percDecayRate = -std::log(ENVELOPE_THRESHOLD / (1.0f - PERCUSSIVE_BODY_DROP)) / decayLength;
    else
        percDecayRate = 0.0f;
}

float EnvelopeGenerator::computeEnvelopeSample() const
{
    switch (currentShape)
    {
        case EnvelopeShape::Exponential:  return computeExponential();
        case EnvelopeShape::Linear:       return computeLinear();
        case EnvelopeShape::Logarithmic:  return computeLogarithmic();
        case EnvelopeShape::Doppler:      return computeExponential();
        case EnvelopeShape::ReverseSawtooth: return computeReverseSawtooth();
        case EnvelopeShape::Gaussian:     return computeGaussian();
        case EnvelopeShape::DoubleTap:    return computeDoubleTap();
        case EnvelopeShape::Percussive:   return computePercussive();
        default:                          return 0.0f;
    }
}

float EnvelopeGenerator::computeExponential() const
{
    const float t = static_cast<float>(sampleIndex);
    return std::exp(-t * expDecayRate);
}

float EnvelopeGenerator::computeLinear() const
{
    const float t = static_cast<float>(sampleIndex);
    const float T = static_cast<float>(tailSamples);
    return 1.0f - (t / T);
}

float EnvelopeGenerator::computeLogarithmic() const
{
    const float t = static_cast<float>(sampleIndex);
    return 1.0f - std::log(1.0f + t * LOG_CURVATURE_K) / logDenominator;
}

float EnvelopeGenerator::computeReverseSawtooth() const
{
    const float t = static_cast<float>(sampleIndex);
    const float T = static_cast<float>(tailSamples);
    if (t >= T)
        return 0.0f;
    return 1.0f - (t / T);
}

float EnvelopeGenerator::computeGaussian() const
{
    const float t = static_cast<float>(sampleIndex);
    const float ratio = t / gaussianSigma;
    return std::exp(-0.5f * ratio * ratio);
}

float EnvelopeGenerator::computeDoubleTap() const
{
    const float t = static_cast<float>(sampleIndex);
    const float tap1 = std::exp(-t * doubleTapDecayRate);

    float tap2 = 0.0f;
    if (t >= doubleTapSpacingSamples)
    {
        const float t2 = t - doubleTapSpacingSamples;
        tap2 = std::exp(-t2 * doubleTapDecayRate);
    }

    return std::max(tap1, tap2);
}

float EnvelopeGenerator::computePercussive() const
{
    const float t = static_cast<float>(sampleIndex);

    if (t < percAttackSamples)
    {
        if (percAttackSamples <= 0.0f)
            return 1.0f;
        return t / percAttackSamples;
    }

    const float tAfterAttack = t - percAttackSamples;

    if (tAfterAttack < percBodySamples)
    {
        if (percBodySamples <= 0.0f)
            return 1.0f - PERCUSSIVE_BODY_DROP;
        return 1.0f - PERCUSSIVE_BODY_DROP * (tAfterAttack / percBodySamples);
    }

    const float tAfterBody = tAfterAttack - percBodySamples;
    return (1.0f - PERCUSSIVE_BODY_DROP) * std::exp(-tAfterBody * percDecayRate);
}
