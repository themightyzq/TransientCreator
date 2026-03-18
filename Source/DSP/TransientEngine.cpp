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

    // Pre-allocate buffers
    dryBuffer.setSize(NUM_CHANNELS, maxBlock);
    wetBuffer.setSize(NUM_CHANNELS, maxBlock);

    // Sine oscillator
    sinePhaseInc = static_cast<float>(440.0 / sampleRate);
    sinePhase = 0.0f;

    // Reset pink noise
    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
        pinkState[i] = 0.0f;

    // Per-sample smoothers
    intensitySmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    intensitySmoothed.setCurrentAndTargetValue(0.75f);

    mixSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    mixSmoothed.setCurrentAndTargetValue(1.0f);

    outputGainSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    outputGainSmoothed.setCurrentAndTargetValue(0.0f);

    transientGainSmoothed.reset(sampleRate, SMOOTHING_TIME_SEC);
    transientGainSmoothed.setCurrentAndTargetValue(0.0f);

    // Output filters — 4D
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

    // Pre-delay — 4G
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

    // Save dry signal for mix processing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

    const bool dopplerActive = (currentShape == EnvelopeShape::Doppler);

    // Per-sample processing into wetBuffer
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float envelopeValue = envelope.getNextSample();

        if (dopplerActive && envelope.consumeTriggerFlag())
        {
            const int tailSamps = static_cast<int>((cachedTailLengthMs / 1000.0f)
                                                    * static_cast<float>(currentSampleRate));
            doppler.trigger(tailSamps);
        }

        const float currentIntensity = intensitySmoothed.getNextValue();
        const float currentMix = mixSmoothed.getNextValue();
        const float currentGainDb = outputGainSmoothed.getNextValue();
        const float currentGainLinear = std::pow(10.0f, currentGainDb / 20.0f);

        // Transient boost — 4B
        const float boostDB = transientGainSmoothed.getNextValue();
        const float boostLinear = std::pow(10.0f, boostDB / 20.0f);

        // Effective amplitude with intensity and transient boost
        const float effectiveAmp = 1.0f - currentIntensity + currentIntensity * envelopeValue;
        const float finalAmp = effectiveAmp * (1.0f + (boostLinear - 1.0f) * envelopeValue);

        // Generate source sample once
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

        // Pre-delay — 4G
        if (preDelaySamples > 0)
        {
            preDelayBuffer[0][static_cast<size_t>(preDelayWriteIndex)] = wetL;
            preDelayBuffer[1][static_cast<size_t>(preDelayWriteIndex)] = wetR;

            const int readIndex = (preDelayWriteIndex - preDelaySamples) & preDelayBufferMask;
            wetL = preDelayBuffer[0][static_cast<size_t>(readIndex)];
            wetR = preDelayBuffer[1][static_cast<size_t>(readIndex)];

            preDelayWriteIndex = (preDelayWriteIndex + 1) & preDelayBufferMask;
        }

        // Mix dry/wet per-sample
        const float dryL = dryBuffer.getSample(0, sample);
        const float dryR = (numChannels > 1) ? dryBuffer.getSample(1, sample) : dryL;

        float outL = dryL * (1.0f - currentMix) + wetL * currentMix;
        float outR = dryR * (1.0f - currentMix) + wetR * currentMix;

        outL *= currentGainLinear;
        outR *= currentGainLinear;

        buffer.setSample(0, sample, outL);
        if (numChannels > 1)
            buffer.setSample(1, sample, outR);
    }

    // Apply output filters — 4D (block-based processing after per-sample loop)
    juce::dsp::AudioBlock<float> block(buffer.getArrayOfWritePointers(),
                                        static_cast<size_t>(numChannels),
                                        static_cast<size_t>(numSamples));
    juce::dsp::ProcessContextReplacing<float> context(block);
    highPassFilter.process(context);
    lowPassFilter.process(context);
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

void TransientEngine::setOutputGain(float dB) { outputGainSmoothed.setTargetValue(dB); }

void TransientEngine::setAttackTime(float ms) { envelope.setAttackTime(ms); }

void TransientEngine::setTransientGain(float dB) { transientGainSmoothed.setTargetValue(dB); }

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
