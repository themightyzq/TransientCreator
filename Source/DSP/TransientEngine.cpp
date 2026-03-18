#include "TransientEngine.h"
#include <cmath>

static constexpr float SMOOTHING_TIME_SEC = 0.02f;

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

    dryBuffer.setSize(NUM_CHANNELS, maxBlock);
    wetBuffer.setSize(NUM_CHANNELS, maxBlock);

    sinePhaseInc = static_cast<float>(440.0 / sampleRate);
    sinePhase = 0.0f;

    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
        pinkState[i] = 0.0f;

    // Per-sample smoothers — gain smoothers in linear space (7B-2)
    intensitySmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    intensitySmoothed.setCurrentAndTargetValue(0.75f);

    mixSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    mixSmoothed.setCurrentAndTargetValue(1.0f);

    outputGainSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    outputGainSmoothed.setCurrentAndTargetValue(1.0f);  // 0dB = linear 1.0

    transientGainSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    transientGainSmoothed.setCurrentAndTargetValue(1.0f);  // 0dB = linear 1.0

    // Output filters
    juce::dsp::ProcessSpec filterSpec;
    filterSpec.sampleRate = sampleRate;
    filterSpec.maximumBlockSize = static_cast<juce::uint32>(maxBlock);
    filterSpec.numChannels = NUM_CHANNELS;

    highPassFilter.prepare(filterSpec);
    highPassFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highPassFilter.setCutoffFrequency(20.0f);

    lowPassFilter.prepare(filterSpec);
    lowPassFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPassFilter.setCutoffFrequency(20000.0f);

    // Pre-delay
    const int maxPreDelaySamples = static_cast<int>((PRE_DELAY_MAX_MS / 1000.0f)
                                                     * static_cast<float>(sampleRate)) + 1;
    preDelayBufferSize = 1;
    while (preDelayBufferSize < maxPreDelaySamples + 1)
        preDelayBufferSize *= 2;
    preDelayBufferMask = preDelayBufferSize - 1;

    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        preDelayBuffer[ch].resize(static_cast<size_t>(preDelayBufferSize), 0.0f);

    preDelayWriteIndex = 0;
    preDelaySamples = 0;
}

void TransientEngine::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const int numChannels = buffer.getNumChannels();

    // 7B-4: Skip dry buffer copy when mix is 100%
    const bool needsDry = (mixSmoothed.getCurrentValue() < 0.999f)
                       || (mixSmoothed.getTargetValue() < 0.999f);

    if (needsDry)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    const bool dopplerActive = (currentShape == EnvelopeShape::Doppler);

    // === Loop 1: Generate wet signal into wetBuffer ===
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float envelopeValue = envelope.getNextSample();

        if (dopplerActive && envelope.consumeTriggerFlag())
        {
            const int tailSamps = static_cast<int>((cachedTailLengthMs / 1000.0f)
                                                    * static_cast<float>(currentSampleRate));
            doppler.trigger(tailSamps);
        }

        // Only intensity and boost advance per-sample in this loop
        const float currentIntensity = intensitySmoothed.getNextValue();
        const float boostLinear = transientGainSmoothed.getNextValue();  // Already linear (7B-2)

        const float effectiveAmp = 1.0f - currentIntensity + currentIntensity * envelopeValue;
        const float finalAmp = effectiveAmp * (1.0f + (boostLinear - 1.0f) * envelopeValue);

        float sourceSample = 0.0f;
        if (inputMode != InputMode::ExternalAudio)
            sourceSample = generateSourceSample();

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

        float wetL = inL * finalAmp;
        float wetR = inR * finalAmp;

        if (dopplerActive)
            doppler.processSampleStereo(wetL, wetR, wetL, wetR);

        if (preDelaySamples > 0)
        {
            preDelayBuffer[0][static_cast<size_t>(preDelayWriteIndex)] = wetL;
            preDelayBuffer[1][static_cast<size_t>(preDelayWriteIndex)] = wetR;

            const int readIndex = (preDelayWriteIndex - preDelaySamples) & preDelayBufferMask;
            wetL = preDelayBuffer[0][static_cast<size_t>(readIndex)];
            wetR = preDelayBuffer[1][static_cast<size_t>(readIndex)];

            preDelayWriteIndex = (preDelayWriteIndex + 1) & preDelayBufferMask;
        }

        // Write wet-only to wetBuffer (7B-1: filters apply to wet only)
        wetBuffer.setSample(0, sample, wetL);
        if (numChannels > 1)
            wetBuffer.setSample(1, sample, wetR);
    }

    // === Apply filters to wet signal only (7B-1) ===
    {
        juce::dsp::AudioBlock<float> wetBlock(wetBuffer.getArrayOfWritePointers(),
                                               static_cast<size_t>(numChannels),
                                               static_cast<size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> wetContext(wetBlock);
        highPassFilter.process(wetContext);
        lowPassFilter.process(wetContext);
    }

    // === Loop 2: Mix filtered wet with dry, apply output gain (7B-1, 7B-4) ===
    if (needsDry)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float currentMix = mixSmoothed.getNextValue();
            const float currentGain = outputGainSmoothed.getNextValue();  // Already linear (7B-2)

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
        // Wet only — just apply gain
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float currentGain = outputGainSmoothed.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
                buffer.setSample(ch, sample, wetBuffer.getSample(ch, sample) * currentGain);
        }
        // Keep mixSmoothed in sync
        mixSmoothed.skip(numSamples);
    }
}

void TransientEngine::reset()
{
    envelope.prepare(currentSampleRate);
    doppler.reset();
    sinePhase = 0.0f;

    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
        pinkState[i] = 0.0f;

    highPassFilter.reset();
    lowPassFilter.reset();

    for (int ch = 0; ch < NUM_CHANNELS; ++ch)
        std::fill(preDelayBuffer[ch].begin(), preDelayBuffer[ch].end(), 0.0f);
    preDelayWriteIndex = 0;
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
    currentShape = shape;
    envelope.setShape(shape);
}

void TransientEngine::setIntensity(float percent)
{
    intensitySmoothed.setTargetValue(percent * PERCENT_TO_FRACTION);
}

void TransientEngine::setPitchShift(float semitones) { doppler.setPitchShiftSemitones(semitones); }

void TransientEngine::setMix(float percent)
{
    mixSmoothed.setTargetValue(percent * PERCENT_TO_FRACTION);
}

void TransientEngine::setInputMode(InputMode mode) { inputMode = mode; }

// 7B-2: Convert dB to linear once per block, smooth in linear space
void TransientEngine::setOutputGain(float dB)
{
    outputGainSmoothed.setTargetValue(std::pow(10.0f, dB / 20.0f));
}

void TransientEngine::setAttackTime(float ms) { envelope.setAttackTime(ms); }

// 7B-2: Convert dB to linear once per block, smooth in linear space
void TransientEngine::setTransientGain(float dB)
{
    transientGainSmoothed.setTargetValue(std::pow(10.0f, dB / 20.0f));
}

void TransientEngine::setTension(float t) { envelope.setTension(t); }
void TransientEngine::setHPFFrequency(float hz) { highPassFilter.setCutoffFrequency(hz); }
void TransientEngine::setLPFFrequency(float hz) { lowPassFilter.setCutoffFrequency(hz); }

void TransientEngine::setSineFrequency(float hz)
{
    sinePhaseInc = static_cast<float>(hz / currentSampleRate);
}

void TransientEngine::setDopplerDirection(DopplerProcessor::Direction dir) { doppler.setDirection(dir); }

void TransientEngine::setPreDelay(float ms)
{
    preDelaySamples = static_cast<int>((ms / 1000.0f) * static_cast<float>(currentSampleRate));
    preDelaySamples = std::min(preDelaySamples, preDelayBufferSize - 1);
}

void TransientEngine::setHumanize(float percent) { envelope.setHumanize(percent); }
void TransientEngine::setSustainHold(float percent) { envelope.setSustainHold(percent); }

// ---------------------------------------------------------------------------
// Internal source generators
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
