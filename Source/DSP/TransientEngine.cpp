#include "TransientEngine.h"
#include <cmath>

static constexpr float SMOOTHING_TIME_SEC = 0.02f;  // 20ms ramp for per-sample smoothing

TransientEngine::TransientEngine()
{
    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
        pinkState[i] = 0.0f;
}

void TransientEngine::prepare(double sampleRate, int maxBlock)
{
    currentSampleRate = sampleRate;
    currentMaxBlockSize = maxBlock;

    envelope.prepare(sampleRate);
    doppler.prepare(sampleRate, maxBlock);

    // Pre-allocate dry buffer for mix processing
    dryBuffer.setSize(2, maxBlock);

    // Sine oscillator phase increment
    sinePhaseInc = static_cast<float>(SINE_FREQUENCY_HZ / sampleRate);
    sinePhase = 0.0f;

    // Reset pink noise state
    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
        pinkState[i] = 0.0f;

    // Initialize per-sample smoothers
    intensitySmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    intensitySmoothed.setCurrentAndTargetValue(0.75f);

    mixSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    mixSmoothed.setCurrentAndTargetValue(1.0f);

    outputGainSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    outputGainSmoothed.setCurrentAndTargetValue(0.0f);  // 0 dB
}

void TransientEngine::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const int numChannels = buffer.getNumChannels();

    // Save dry signal for mix processing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    const bool dopplerActive = (currentShape == EnvelopeShape::Doppler);
    bool wasInTail = envelope.isInTail();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Detect tail re-trigger to sync DopplerProcessor
        const bool nowInTail = envelope.isInTail();
        if (dopplerActive && nowInTail && !wasInTail)
        {
            const int tailSamps = static_cast<int>((cachedTailLengthMs / 1000.0f)
                                                    * static_cast<float>(currentSampleRate));
            doppler.trigger(tailSamps);
        }
        wasInTail = nowInTail;

        // Get envelope amplitude for this sample
        const float envelopeValue = envelope.getNextSample();

        // Per-sample smoothed values
        const float currentIntensity = intensitySmoothed.getNextValue();
        const float currentMix = mixSmoothed.getNextValue();
        const float currentGainDb = outputGainSmoothed.getNextValue();
        const float currentGainLinear = std::pow(10.0f, currentGainDb / 20.0f);

        // Compute effective amplitude: blend between passthrough (1.0) and envelope
        const float effectiveAmp = 1.0f - currentIntensity + currentIntensity * envelopeValue;

        // Fix 1C: Generate internal source sample ONCE per sample step
        float sourceSample = 0.0f;
        if (inputMode != InputMode::ExternalAudio)
            sourceSample = generateSourceSample();

        // Gather input for both channels
        float inL, inR;
        if (inputMode == InputMode::ExternalAudio)
        {
            inL = buffer.getSample(0, sample);
            inR = (numChannels > 1) ? buffer.getSample(1, sample) : inL;
        }
        else
        {
            inL = sourceSample;
            inR = sourceSample;
        }

        // Apply transient envelope
        float wetL = inL * effectiveAmp;
        float wetR = inR * effectiveAmp;

        // Fix 1B: Doppler processes both channels together — sweep advances once
        if (dopplerActive)
            doppler.processSampleStereo(wetL, wetR, wetL, wetR);

        // Apply per-sample dry/wet mix
        const float dryL = dryBuffer.getSample(0, sample);
        const float dryR = (numChannels > 1) ? dryBuffer.getSample(1, sample) : dryL;

        float outL = dryL * (1.0f - currentMix) + wetL * currentMix;
        float outR = dryR * (1.0f - currentMix) + wetR * currentMix;

        // Apply per-sample output gain
        outL *= currentGainLinear;
        outR *= currentGainLinear;

        // Write output
        buffer.setSample(0, sample, outL);
        if (numChannels > 1)
            buffer.setSample(1, sample, outR);
    }
}

void TransientEngine::reset()
{
    envelope.prepare(currentSampleRate);
    doppler.reset();
    sinePhase = 0.0f;

    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
        pinkState[i] = 0.0f;
}

// ---------------------------------------------------------------------------
// Parameter setters — set target values for smoothers, or update discrete state
// ---------------------------------------------------------------------------

void TransientEngine::setTailLength(float ms)
{
    cachedTailLengthMs = ms;
    envelope.setTailLength(ms);
}

void TransientEngine::setSilenceGap(float ms)
{
    envelope.setSilenceGap(ms);
}

void TransientEngine::setShape(EnvelopeShape shape)
{
    currentShape = shape;
    envelope.setShape(shape);
}

void TransientEngine::setIntensity(float percent)
{
    intensitySmoothed.setTargetValue(percent * PERCENT_TO_FRACTION);
}

void TransientEngine::setPitchShift(float semitones)
{
    doppler.setPitchShiftSemitones(semitones);
}

void TransientEngine::setMix(float percent)
{
    mixSmoothed.setTargetValue(percent * PERCENT_TO_FRACTION);
}

void TransientEngine::setInputMode(InputMode mode)
{
    inputMode = mode;
}

void TransientEngine::setOutputGain(float dB)
{
    outputGainSmoothed.setTargetValue(dB);
}

// ---------------------------------------------------------------------------
// Internal source generators — called once per sample step (Fix 1C)
// ---------------------------------------------------------------------------

float TransientEngine::generateSourceSample()
{
    switch (inputMode)
    {
        case InputMode::WhiteNoise:     return generateWhiteNoise();
        case InputMode::PinkNoise:      return generatePinkNoise();
        case InputMode::SineOscillator: return generateSine();
        case InputMode::ExternalAudio:
        default:                        return 0.0f;
    }
}

float TransientEngine::generateWhiteNoise()
{
    return noiseRng.nextFloat() * 2.0f - 1.0f;
}

float TransientEngine::generatePinkNoise()
{
    const float white = generateWhiteNoise();

    pinkState[0] = 0.99886f * pinkState[0] + white * 0.0555179f;
    pinkState[1] = 0.99332f * pinkState[1] + white * 0.0750759f;
    pinkState[2] = 0.96900f * pinkState[2] + white * 0.1538520f;
    pinkState[3] = 0.86650f * pinkState[3] + white * 0.3104856f;
    pinkState[4] = 0.55000f * pinkState[4] + white * 0.5329522f;
    pinkState[5] = -0.7616f * pinkState[5] - white * 0.0168980f;

    float pink = pinkState[0] + pinkState[1] + pinkState[2] + pinkState[3]
               + pinkState[4] + pinkState[5] + pinkState[6] + white * 0.5362f;

    pinkState[6] = white * 0.115926f;

    static constexpr float PINK_NOISE_GAIN = 0.11f;
    return pink * PINK_NOISE_GAIN;
}

float TransientEngine::generateSine()
{
    const float sample = std::sin(sinePhase * juce::MathConstants<float>::twoPi);
    sinePhase += sinePhaseInc;

    if (sinePhase >= 1.0f)
        sinePhase -= 1.0f;

    return sample;
}
