#include "TransientEngine.h"
#include <cmath>

static constexpr float SMOOTHING_TIME_SEC = 0.02f;
static constexpr float PINK_NOISE_GAIN = 0.11f;

TransientEngine::TransientEngine()
{
    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
    {
        pinkStateL[i] = 0.0f;
        pinkStateR[i] = 0.0f;
    }
}

void TransientEngine::prepare(double sampleRate, int maxBlock)
{
    currentSampleRate = sampleRate;
    currentMaxBlockSize = maxBlock;

    envelope.prepare(sampleRate);
    doppler.prepare(sampleRate, maxBlock);

    dryBuffer.setSize(NUM_CHANNELS, maxBlock);
    wetBuffer.setSize(NUM_CHANNELS, maxBlock);

    sinePhaseInc = static_cast<float>(440.0 / sampleRate);
    sinePhase = 0.0f;

    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
    {
        pinkStateL[i] = 0.0f;
        pinkStateR[i] = 0.0f;
    }

    mixSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    mixSmoothed.setCurrentAndTargetValue(1.0f);

    outputGainSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    outputGainSmoothed.setCurrentAndTargetValue(1.0f);

    transientGainSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    transientGainSmoothed.setCurrentAndTargetValue(1.0f);

}

void TransientEngine::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const int numChannels = buffer.getNumChannels();

    const bool needsDry = (mixSmoothed.getCurrentValue() < 0.999f)
                       || (mixSmoothed.getTargetValue() < 0.999f);

    if (needsDry)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // Doppler is active when pitch shift > threshold (independent of shape)
    const bool dopplerActive = doppler.isActive();

    // === Loop 1: Generate wet signal into wetBuffer ===
    // Signal order: Input → Doppler (pitch) → Envelope × Boost → Pre-delay
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float envelopeValue = envelope.getNextSample();

        if (dopplerActive && envelope.consumeTriggerFlag())
        {
            const int tailSamps = static_cast<int>((cachedTailLengthMs / 1000.0f)
                                                    * static_cast<float>(currentSampleRate));
            doppler.trigger(tailSamps);
        }

        // Get raw input audio
        float inL, inR;
        if (inputMode == InputMode::ExternalAudio)
        {
            inL = buffer.getSample(0, sample);
            inR = (numChannels > 1) ? buffer.getSample(1, sample) : inL;
        }
        else
        {
            generateSourceSampleStereo(inL, inR);
        }

        // Doppler pitch-shifts the RAW audio BEFORE envelope shaping
        if (dopplerActive)
            doppler.processSampleStereo(inL, inR, inL, inR);

        // Envelope shapes amplitude AFTER pitch — transient timing is fixed
        const float boostLinear = transientGainSmoothed.getNextValue();
        const float finalAmp = envelopeValue * boostLinear;
        float wetL = inL * finalAmp;
        float wetR = inR * finalAmp;

        wetBuffer.setSample(0, sample, wetL);
        if (numChannels > 1)
            wetBuffer.setSample(1, sample, wetR);
    }

    // === Loop 2: Mix filtered wet with dry, apply output gain ===
    if (needsDry)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float currentMix = mixSmoothed.getNextValue();
            const float currentGain = outputGainSmoothed.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                const float dry = dryBuffer.getSample(ch, sample);
                const float wet = wetBuffer.getSample(ch, sample);
                buffer.setSample(ch, sample, (dry * (1.0f - currentMix) + wet * currentMix) * currentGain);
            }
        }
    }
    else
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float currentGain = outputGainSmoothed.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample(ch, sample, wetBuffer.getSample(ch, sample) * currentGain);
        }
        mixSmoothed.skip(numSamples);
    }
}

void TransientEngine::reset()
{
    envelope.prepare(currentSampleRate);
    doppler.reset();
    sinePhase = 0.0f;

    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
    {
        pinkStateL[i] = 0.0f;
        pinkStateR[i] = 0.0f;
    }

}

// ---------------------------------------------------------------------------
// Parameter setters
// ---------------------------------------------------------------------------

void TransientEngine::setTailLength(float ms)
{
    cachedTailLengthMs = ms;
    envelope.setTailLength(ms);
}

void TransientEngine::setSilenceGap(float ms) { envelope.setSilenceGap(ms); }

void TransientEngine::setShape(EnvelopeShape shape)
{
    envelope.setShape(shape);
}

void TransientEngine::setPitchStart(float semitones)
{
    doppler.setPitchStart(semitones);
}

void TransientEngine::setPitchEnd(float semitones)
{
    doppler.setPitchEnd(semitones);
}

void TransientEngine::setMix(float percent)
{
    mixSmoothed.setTargetValue(percent * PERCENT_TO_FRACTION);
}

void TransientEngine::setInputMode(InputMode mode) { inputMode = mode; }

void TransientEngine::setOutputGain(float dB)
{
    outputGainSmoothed.setTargetValue(std::pow(10.0f, dB / 20.0f));
}

void TransientEngine::setAttackTime(float ms) { envelope.setAttackTime(ms); }

void TransientEngine::setTransientGain(float dB)
{
    transientGainSmoothed.setTargetValue(std::pow(10.0f, dB / 20.0f));
}

void TransientEngine::setSineFrequency(float hz)
{
    sinePhaseInc = static_cast<float>(hz / currentSampleRate);
}

void TransientEngine::setHumanize(float percent) { envelope.setHumanize(percent); }
void TransientEngine::setSustainHold(float percent) { envelope.setSustainHold(percent); }

void TransientEngine::setCustomCurve(const float* lut, int size)
{
    envelope.setCustomCurve(lut, size);
}

float TransientEngine::getPlayheadPosition() const
{
    return envelope.getNormalizedCyclePosition();
}

bool TransientEngine::getIsInTail() const
{
    return envelope.isInTail();
}

// ---------------------------------------------------------------------------
// Internal source generators — stereo (independent L/R for noise)
// ---------------------------------------------------------------------------

void TransientEngine::generateSourceSampleStereo(float& outL, float& outR)
{
    switch (inputMode)
    {
        case InputMode::WhiteNoise:
            outL = noiseRngL.nextFloat() * 2.0f - 1.0f;
            outR = noiseRngR.nextFloat() * 2.0f - 1.0f;
            return;

        case InputMode::PinkNoise:
        {
            // Independent L/R pink noise via Paul Kellet's algorithm
            auto pinkSample = [](juce::Random& rng, float* state) -> float
            {
                const float white = rng.nextFloat() * 2.0f - 1.0f;
                state[0] = 0.99886f * state[0] + white * 0.0555179f;
                state[1] = 0.99332f * state[1] + white * 0.0750759f;
                state[2] = 0.96900f * state[2] + white * 0.1538520f;
                state[3] = 0.86650f * state[3] + white * 0.3104856f;
                state[4] = 0.55000f * state[4] + white * 0.5329522f;
                state[5] = -0.7616f * state[5] - white * 0.0168980f;
                float pink = state[0] + state[1] + state[2] + state[3]
                           + state[4] + state[5] + state[6] + white * 0.5362f;
                state[6] = white * 0.115926f;
                return pink * PINK_NOISE_GAIN;
            };

            outL = pinkSample(noiseRngL, pinkStateL);
            outR = pinkSample(noiseRngR, pinkStateR);
            return;
        }

        case InputMode::SineOscillator:
        {
            const float s = generateSine();
            outL = s;
            outR = s;
            return;
        }

        case InputMode::ExternalAudio:
        default:
            outL = 0.0f;
            outR = 0.0f;
            return;
    }
}

float TransientEngine::generateWhiteNoise()
{
    return noiseRngL.nextFloat() * 2.0f - 1.0f;
}

float TransientEngine::generatePinkNoiseSample()
{
    const float white = generateWhiteNoise();

    pinkStateL[0] = 0.99886f * pinkStateL[0] + white * 0.0555179f;
    pinkStateL[1] = 0.99332f * pinkStateL[1] + white * 0.0750759f;
    pinkStateL[2] = 0.96900f * pinkStateL[2] + white * 0.1538520f;
    pinkStateL[3] = 0.86650f * pinkStateL[3] + white * 0.3104856f;
    pinkStateL[4] = 0.55000f * pinkStateL[4] + white * 0.5329522f;
    pinkStateL[5] = -0.7616f * pinkStateL[5] - white * 0.0168980f;

    float pink = pinkStateL[0] + pinkStateL[1] + pinkStateL[2] + pinkStateL[3]
               + pinkStateL[4] + pinkStateL[5] + pinkStateL[6] + white * 0.5362f;

    pinkStateL[6] = white * 0.115926f;

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
