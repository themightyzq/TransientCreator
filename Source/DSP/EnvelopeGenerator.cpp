#include "EnvelopeGenerator.h"

using namespace EnvelopeConstants;

// ---------------------------------------------------------------------------
// Helper: read from a LUT with linear interpolation
// ---------------------------------------------------------------------------
static float readLUT(const float* lut, int size, float normalizedPos)
{
    if (lut == nullptr || size < 2) return 0.0f;
    const float pos = normalizedPos * static_cast<float>(size - 1);
    const int idx = static_cast<int>(pos);
    const float frac = pos - static_cast<float>(idx);
    if (idx >= size - 1) return lut[size - 1];
    return lut[idx] * (1.0f - frac) + lut[idx + 1] * frac;
}

void EnvelopeGenerator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    cachedTailLengthMs = -1.0f;
    cachedSilenceGapMs = -1.0f;
    cachedAttackTimeMs = -1.0f;
    cachedSustainHoldPercent = -1.0f;
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

        // Minimum onset smoothing: when attack is 0 and we just started from silence,
        // apply a short linear fade to prevent click
        if (attackSamples == 0 && !crossfading && sampleIndex < MIN_ONSET_FADE_SAMPLES)
        {
            const float phase = static_cast<float>(sampleIndex + 1) / static_cast<float>(MIN_ONSET_FADE_SAMPLES + 1);
            const float fadeFactor = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * phase));
            output *= fadeFactor;
        }

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

            // Minimum onset smoothing for auto-retrigger from silence
            if (attackSamples == 0 && !crossfading && sampleIndex < MIN_ONSET_FADE_SAMPLES)
            {
                const float phase = static_cast<float>(sampleIndex + 1) / static_cast<float>(MIN_ONSET_FADE_SAMPLES + 1);
                const float fadeFactor = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * phase));
                output *= fadeFactor;
            }

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

float EnvelopeGenerator::getNormalizedCyclePosition() const
{
    const int totalSamples = tailSamples + gapSamples;
    if (totalSamples <= 0) return 0.0f;

    if (currentState == State::Tail)
        return static_cast<float>(sampleIndex) / static_cast<float>(totalSamples);
    else
        return static_cast<float>(tailSamples + sampleIndex) / static_cast<float>(totalSamples);
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

void EnvelopeGenerator::setShape(EnvelopeShape shape) { currentShape = shape; }
void EnvelopeGenerator::setAttackTime(float ms)
{
    if (std::fabs(ms - cachedAttackTimeMs) < 1.0e-6f) return;
    attackTimeMs = ms;
    cachedAttackTimeMs = ms;
    recalculateSampleCounts();
}
void EnvelopeGenerator::setHumanize(float percent) { humanizeAmount = percent * 0.01f; }

void EnvelopeGenerator::setSustainHold(float percent)
{
    if (std::fabs(percent - cachedSustainHoldPercent) < 1.0e-6f) return;
    sustainHoldPercent = percent;
    cachedSustainHoldPercent = percent;
    recalculateSampleCounts();
}

void EnvelopeGenerator::setCustomCurve(const float* lut, int size)
{
    customCurveLUT = lut;
    customCurveSize = size;
}

// ---------------------------------------------------------------------------
// Static visualization method — always reads from LUT for decay
// ---------------------------------------------------------------------------

float EnvelopeGenerator::computeShapeAtNormalized(float normalizedPos, EnvelopeShape /*shape*/,
                                                   float attackFraction, float holdFrac,
                                                   const float* customLUT, int customLUTSize)
{
    // Attack phase: raised-cosine ramp
    if (attackFraction > 0.0f && normalizedPos < attackFraction)
    {
        const float attackProgress = normalizedPos / attackFraction;
        return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi * attackProgress));
    }

    // Hold phase: flat at 1.0
    const float holdEnd = attackFraction + holdFrac;
    if (holdFrac > 0.0f && normalizedPos < holdEnd)
        return 1.0f;

    // Decay phase: always read from LUT
    const float decayFraction = 1.0f - holdEnd;
    const float decayNorm = (decayFraction > 0.0f) ? (normalizedPos - holdEnd) / decayFraction : 0.0f;
    return readLUT(customLUT, customLUTSize, decayNorm);
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

    doubleTapSpacingSamples = D * DOUBLE_TAP_SPACING;
    doubleTapDecayRate = -std::log(DOUBLE_TAP_THRESHOLD) / (D * DOUBLE_TAP_DECAY_SPAN);

    percBodySamples   = D * PERCUSSIVE_BODY_RATIO;
    const float decayLength = D - percBodySamples;
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
        return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::pi
                       * static_cast<float>(sampleIndex) / static_cast<float>(attackSamples)));
    }

    // Hold phase: flat at 1.0
    if (holdSamples > 0 && sampleIndex < (attackSamples + holdSamples))
        return 1.0f;

    // Decay phase — always read from custom curve LUT
    const int decayIndex = sampleIndex - attackSamples - holdSamples;
    const float D = static_cast<float>(std::max(1, decaySamples));
    const float normalizedDt = static_cast<float>(decayIndex) / D;
    return readLUT(customCurveLUT, customCurveSize, normalizedDt);
}
