#include "EnvelopeGenerator.h"

void EnvelopeGenerator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    recalculateSampleCounts();

    // Reset state
    currentState = State::Silence;
    sampleIndex = 0;
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
            // Transition to silence
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
            // Re-trigger
            trigger();

            // Return the first sample of the new tail
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

bool EnvelopeGenerator::isActive() const
{
    return currentState == State::Tail;
}

bool EnvelopeGenerator::isInTail() const
{
    return currentState == State::Tail;
}

void EnvelopeGenerator::setTailLength(float ms)
{
    tailLengthMs = ms;
    recalculateSampleCounts();
}

void EnvelopeGenerator::setSilenceGap(float ms)
{
    silenceGapMs = ms;
    recalculateSampleCounts();
}

void EnvelopeGenerator::setShape(EnvelopeShape shape)
{
    currentShape = shape;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void EnvelopeGenerator::recalculateSampleCounts()
{
    tailSamples = std::max(1, static_cast<int>((tailLengthMs / 1000.0f) * currentSampleRate));
    gapSamples  = std::max(0, static_cast<int>((silenceGapMs / 1000.0f) * currentSampleRate));

    const float T = static_cast<float>(tailSamples);

    // Exponential / Doppler decay rate: envelope reaches ENVELOPE_THRESHOLD at end of tail
    // decayRate = -ln(threshold) / T
    expDecayRate = -std::log(ENVELOPE_THRESHOLD) / T;

    // Logarithmic: precompute denominator log(1 + T * k)
    logDenominator = std::log(1.0f + T * LOG_CURVATURE_K);

    // Gaussian: sigma = T / 4 (places ~95% of energy in first half)
    gaussianSigma = T * GAUSSIAN_SIGMA_RATIO;

    // Double Tap: spacing and per-tap decay
    doubleTapSpacingSamples = T * DOUBLE_TAP_SPACING;
    // Each tap decays within its own half of the tail
    doubleTapDecayRate = -std::log(ENVELOPE_THRESHOLD) / (T * 0.5f);

    // Percussive: attack, body, and decay segments
    percAttackSamples = static_cast<float>(PERCUSSIVE_ATTACK_S * currentSampleRate);
    percBodySamples   = T * PERCUSSIVE_BODY_RATIO;
    // Decay segment starts at (1.0 - PERCUSSIVE_BODY_DROP) amplitude and spans remaining tail
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
        case EnvelopeShape::Doppler:      return computeExponential(); // Amplitude same as exponential; pitch handled by DopplerProcessor
        case EnvelopeShape::ReverseSawtooth: return computeReverseSawtooth();
        case EnvelopeShape::Gaussian:     return computeGaussian();
        case EnvelopeShape::DoubleTap:    return computeDoubleTap();
        case EnvelopeShape::Percussive:   return computePercussive();
        default:                          return 0.0f;
    }
}

float EnvelopeGenerator::computeExponential() const
{
    // amplitude = exp(-t * decayRate)
    const float t = static_cast<float>(sampleIndex);
    return std::exp(-t * expDecayRate);
}

float EnvelopeGenerator::computeLinear() const
{
    // amplitude = 1.0 - (t / T)
    const float t = static_cast<float>(sampleIndex);
    const float T = static_cast<float>(tailSamples);
    return 1.0f - (t / T);
}

float EnvelopeGenerator::computeLogarithmic() const
{
    // amplitude = 1.0 - log(1 + t * k) / log(1 + T * k)
    const float t = static_cast<float>(sampleIndex);
    return 1.0f - std::log(1.0f + t * LOG_CURVATURE_K) / logDenominator;
}

float EnvelopeGenerator::computeReverseSawtooth() const
{
    // Same shape as linear, but with hard cutoff (no end-of-tail smoothing)
    const float t = static_cast<float>(sampleIndex);
    const float T = static_cast<float>(tailSamples);
    if (t >= T)
        return 0.0f;
    return 1.0f - (t / T);
}

float EnvelopeGenerator::computeGaussian() const
{
    // amplitude = exp(-0.5 * (t / sigma)^2)
    const float t = static_cast<float>(sampleIndex);
    const float ratio = t / gaussianSigma;
    return std::exp(-0.5f * ratio * ratio);
}

float EnvelopeGenerator::computeDoubleTap() const
{
    // Two exponential pulses: tap1 starts at t=0, tap2 starts at t=tapSpacing
    // amplitude = max(tap1, tap2)
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
    // Three segments: attack (linear ramp up), body (slight linear drop), decay (exponential)
    const float t = static_cast<float>(sampleIndex);

    // Attack: linear ramp from 0 to 1
    if (t < percAttackSamples)
    {
        if (percAttackSamples <= 0.0f)
            return 1.0f;
        return t / percAttackSamples;
    }

    const float tAfterAttack = t - percAttackSamples;

    // Body: linear drop from 1.0 to (1.0 - PERCUSSIVE_BODY_DROP)
    if (tAfterAttack < percBodySamples)
    {
        if (percBodySamples <= 0.0f)
            return 1.0f - PERCUSSIVE_BODY_DROP;
        return 1.0f - PERCUSSIVE_BODY_DROP * (tAfterAttack / percBodySamples);
    }

    // Decay: exponential from (1.0 - PERCUSSIVE_BODY_DROP) toward zero
    const float tAfterBody = tAfterAttack - percBodySamples;
    return (1.0f - PERCUSSIVE_BODY_DROP) * std::exp(-tAfterBody * percDecayRate);
}
