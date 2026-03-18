#include "EnvelopeGenerator.h"

using namespace EnvelopeConstants;

void EnvelopeGenerator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    cachedTailLengthMs = -1.0f;
    cachedSilenceGapMs = -1.0f;
    recalculateSampleCounts();

    currentState = State::Silence;
    sampleIndex = 0;
    justTriggered = false;
    crossfading = false;
    crossfadeIndex = 0;
    previousEndAmplitude = 0.0f;
}

void EnvelopeGenerator::trigger()
{
    if (currentState == State::Silence)
        previousEndAmplitude = 0.0f;
    else
        previousEndAmplitude = computeEnvelopeSample();

    currentState = State::Tail;
    sampleIndex = 0;
    justTriggered = true;

    if (humanizeAmount > 0.0f)
    {
        const float variation = humanizeAmount * HUMANIZE_VARIATION;
        const float tailJitter = 1.0f + (humanizeRng.nextFloat() * 2.0f - 1.0f) * variation;
        const float gapJitter  = 1.0f + (humanizeRng.nextFloat() * 2.0f - 1.0f) * variation;

        tailSamples = std::max(1, static_cast<int>(static_cast<float>(tailSamples) * tailJitter));
        gapSamples  = std::max(0, static_cast<int>(static_cast<float>(gapSamples) * gapJitter));

        recalculateCoefficients();
    }

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

bool EnvelopeGenerator::isInTail() const { return currentState == State::Tail; }

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

void EnvelopeGenerator::setShape(EnvelopeShape shape) { currentShape = shape; }
void EnvelopeGenerator::setAttackTime(float ms) { attackTimeMs = ms; recalculateSampleCounts(); }
void EnvelopeGenerator::setTension(float t) { tension = t; }
void EnvelopeGenerator::setHumanize(float percent) { humanizeAmount = percent * 0.01f; }

void EnvelopeGenerator::setSustainHold(float percent)
{
    sustainHoldPercent = percent;
    recalculateSampleCounts();
}

// ---------------------------------------------------------------------------
// Static visualization method
// ---------------------------------------------------------------------------

float EnvelopeGenerator::computeShapeAtNormalized(float normalizedPos, EnvelopeShape shape,
                                                   float referenceTailSamples, float tens,
                                                   float attackFraction, float holdFrac)
{
    // Attack phase: raised-cosine ramp
    if (attackFraction > 0.0f && normalizedPos < attackFraction)
    {
        const float attackProgress = normalizedPos / attackFraction;
        float attackEnv = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * attackProgress));
        if (std::fabs(tens - 1.0f) > 1.0e-6f)
            attackEnv = std::pow(std::max(0.0f, attackEnv), tens);
        return attackEnv;
    }

    // Hold phase: flat at 1.0
    const float holdEnd = attackFraction + holdFrac;
    if (holdFrac > 0.0f && normalizedPos < holdEnd)
        return 1.0f;

    // Decay phase: remap to 0.0–1.0 over remaining portion
    const float decayFraction = 1.0f - holdEnd;
    const float decayNorm = (decayFraction > 0.0f) ? (normalizedPos - holdEnd) / decayFraction : 0.0f;
    const float t = decayNorm * referenceTailSamples;
    const float T = referenceTailSamples;

    float raw = 0.0f;

    switch (shape)
    {
        case EnvelopeShape::Exponential:
        case EnvelopeShape::Doppler:
        {
            const float decayRate = -std::log(ENVELOPE_THRESHOLD) / T;
            raw = std::exp(-t * decayRate);
            break;
        }
        case EnvelopeShape::Linear:
            raw = std::max(0.0f, 1.0f - (t / T));
            break;
        case EnvelopeShape::Logarithmic:
        {
            const float normalizedT = t / T;
            raw = std::pow(std::max(0.0f, 1.0f - normalizedT), LOG_POWER_CURVATURE);
            break;
        }
        case EnvelopeShape::ReverseSawtooth:
        {
            const float holdEnd2 = T * REVERSE_SAW_HOLD_FRACTION;
            if (t < holdEnd2)
                raw = 1.0f;
            else
                raw = std::max(0.0f, 1.0f - (t - holdEnd2) / (T - holdEnd2));
            break;
        }
        case EnvelopeShape::Gaussian:
        {
            const float sigma = T * GAUSSIAN_SIGMA_RATIO;
            const float ratio = t / sigma;
            raw = std::exp(-0.5f * ratio * ratio);
            break;
        }
        case EnvelopeShape::DoubleTap:
        {
            const float decayRate = -std::log(DOUBLE_TAP_THRESHOLD) / (T * DOUBLE_TAP_DECAY_SPAN);
            const float tap1 = std::exp(-t * decayRate);
            float tap2 = 0.0f;
            const float spacing = T * DOUBLE_TAP_SPACING;
            if (t >= spacing)
                tap2 = std::exp(-(t - spacing) * decayRate);
            raw = std::max(tap1, tap2);
            break;
        }
        case EnvelopeShape::Percussive:
        {
            static constexpr float REFERENCE_RATE = 44100.0f;
            const float percAttack = PERCUSSIVE_ATTACK_S * REFERENCE_RATE;
            const float percBody = T * PERCUSSIVE_BODY_RATIO;

            if (t < percAttack)
                raw = (percAttack > 0.0f) ? (t / percAttack) : 1.0f;
            else
            {
                const float tAfterAttack = t - percAttack;
                if (tAfterAttack < percBody)
                    raw = (percBody > 0.0f) ? (1.0f - PERCUSSIVE_BODY_DROP * (tAfterAttack / percBody)) : (1.0f - PERCUSSIVE_BODY_DROP);
                else
                {
                    const float tAfterBody = tAfterAttack - percBody;
                    const float decayLen = T - percAttack - percBody;
                    const float decayRate = (decayLen > 0.0f)
                        ? -std::log(ENVELOPE_THRESHOLD / (1.0f - PERCUSSIVE_BODY_DROP)) / decayLen : 0.0f;
                    raw = (1.0f - PERCUSSIVE_BODY_DROP) * std::exp(-tAfterBody * decayRate);
                }
            }
            break;
        }
        default:
            break;
    }

    if (std::fabs(tens - 1.0f) > 1.0e-6f)
        raw = std::pow(std::max(0.0f, raw), tens);

    return raw;
}

// ---------------------------------------------------------------------------
// Private
// ---------------------------------------------------------------------------

void EnvelopeGenerator::recalculateSampleCounts()
{
    tailSamples   = std::max(1, static_cast<int>((tailLengthMs / 1000.0f) * currentSampleRate));
    gapSamples    = std::max(0, static_cast<int>((silenceGapMs / 1000.0f) * currentSampleRate));
    attackSamples = std::max(0, std::min(
        static_cast<int>((attackTimeMs / 1000.0f) * currentSampleRate),
        tailSamples - 1));

    holdSamples = static_cast<int>((sustainHoldPercent / 100.0f) * static_cast<float>(tailSamples));
    holdSamples = std::max(0, std::min(holdSamples, tailSamples - attackSamples - 1));

    decaySamples = tailSamples - attackSamples - holdSamples;

    recalculateCoefficients();
}

void EnvelopeGenerator::recalculateCoefficients()
{
    const float D = static_cast<float>(std::max(1, decaySamples));

    expDecayRate = -std::log(ENVELOPE_THRESHOLD) / D;
    gaussianSigma = D * GAUSSIAN_SIGMA_RATIO;

    doubleTapSpacingSamples = D * DOUBLE_TAP_SPACING;
    doubleTapDecayRate = -std::log(DOUBLE_TAP_THRESHOLD) / (D * DOUBLE_TAP_DECAY_SPAN);

    percAttackSamples = static_cast<float>(PERCUSSIVE_ATTACK_S * currentSampleRate);
    percBodySamples   = D * PERCUSSIVE_BODY_RATIO;
    const float decayLength = D - percAttackSamples - percBodySamples;
    if (decayLength > 0.0f)
        percDecayRate = -std::log(ENVELOPE_THRESHOLD / (1.0f - PERCUSSIVE_BODY_DROP)) / decayLength;
    else
        percDecayRate = 0.0f;
}

float EnvelopeGenerator::computeEnvelopeSample() const
{
    // Attack phase: raised-cosine ramp
    if (attackSamples > 0 && sampleIndex < attackSamples)
    {
        float attackEnv = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi
                          * static_cast<float>(sampleIndex) / static_cast<float>(attackSamples)));
        if (std::fabs(tension - 1.0f) > 1.0e-6f)
            attackEnv = std::pow(std::max(0.0f, attackEnv), tension);
        return attackEnv;
    }

    // Hold phase: flat at 1.0
    if (holdSamples > 0 && sampleIndex < (attackSamples + holdSamples))
        return 1.0f;

    // Decay phase
    const int decayIndex = sampleIndex - attackSamples - holdSamples;
    const float D = static_cast<float>(std::max(1, decaySamples));

    float raw = 0.0f;

    switch (currentShape)
    {
        case EnvelopeShape::Exponential:
        case EnvelopeShape::Doppler:
            raw = std::exp(-static_cast<float>(decayIndex) * expDecayRate);
            break;

        case EnvelopeShape::Linear:
            raw = std::max(0.0f, 1.0f - (static_cast<float>(decayIndex) / D));
            break;

        case EnvelopeShape::Logarithmic:
        {
            const float normalizedT = static_cast<float>(decayIndex) / D;
            raw = std::pow(std::max(0.0f, 1.0f - normalizedT), LOG_POWER_CURVATURE);
            break;
        }

        case EnvelopeShape::ReverseSawtooth:
        {
            const float dt = static_cast<float>(decayIndex);
            const float holdEnd2 = D * REVERSE_SAW_HOLD_FRACTION;
            if (dt < holdEnd2)
                raw = 1.0f;
            else
                raw = std::max(0.0f, 1.0f - (dt - holdEnd2) / (D - holdEnd2));
            break;
        }

        case EnvelopeShape::Gaussian:
        {
            const float ratio = static_cast<float>(decayIndex) / gaussianSigma;
            raw = std::exp(-0.5f * ratio * ratio);
            break;
        }

        case EnvelopeShape::DoubleTap:
        {
            const float dt = static_cast<float>(decayIndex);
            const float tap1 = std::exp(-dt * doubleTapDecayRate);
            float tap2 = 0.0f;
            if (dt >= doubleTapSpacingSamples)
                tap2 = std::exp(-(dt - doubleTapSpacingSamples) * doubleTapDecayRate);
            raw = std::max(tap1, tap2);
            break;
        }

        case EnvelopeShape::Percussive:
        {
            const float dt = static_cast<float>(decayIndex);
            if (dt < percAttackSamples)
                raw = (percAttackSamples > 0.0f) ? (dt / percAttackSamples) : 1.0f;
            else
            {
                const float tAfterAttack = dt - percAttackSamples;
                if (tAfterAttack < percBodySamples)
                    raw = (percBodySamples > 0.0f) ? (1.0f - PERCUSSIVE_BODY_DROP * (tAfterAttack / percBodySamples)) : (1.0f - PERCUSSIVE_BODY_DROP);
                else
                    raw = (1.0f - PERCUSSIVE_BODY_DROP) * std::exp(-(tAfterAttack - percBodySamples) * percDecayRate);
            }
            break;
        }

        default:
            break;
    }

    if (std::fabs(tension - 1.0f) > 1.0e-6f)
        raw = std::pow(std::max(0.0f, raw), tension);

    return raw;
}
