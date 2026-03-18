#include "TransientEngine.h"
#include <cmath>

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
}

void TransientEngine::processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    const int numChannels = buffer.getNumChannels();

    // Save dry signal for mix processing (only if mix < 1.0)
    if (mix < 1.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    const bool dopplerActive = (currentShape == EnvelopeShape::Doppler);
    bool wasInTail = envelope.isInTail();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Detect tail re-trigger to sync DopplerProcessor
        const bool nowInTail = envelope.isInTail();
        if (dopplerActive && nowInTail && !wasInTail)
        {
            const int tailSamples = static_cast<int>((cachedTailLengthMs / 1000.0f)
                                                      * static_cast<float>(currentSampleRate));
            doppler.trigger(tailSamples);
        }
        wasInTail = nowInTail;

        // Get envelope amplitude for this sample
        const float envelopeValue = envelope.getNextSample();

        // Compute effective amplitude: blend between passthrough (1.0) and envelope
        // At intensity=0%: effectiveAmp = 1.0 (no effect)
        // At intensity=100%: effectiveAmp = envelopeValue (full effect)
        const float effectiveAmp = 1.0f - intensity + intensity * envelopeValue;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float inputSample = generateInputSample(ch);

            // For external audio, use the buffer's audio
            if (inputMode == InputMode::ExternalAudio)
                inputSample = buffer.getSample(ch, sample);

            // Apply transient envelope
            float wetSample = inputSample * effectiveAmp;

            // Apply Doppler pitch-shift (per-channel processing through delay line)
            if (dopplerActive)
                wetSample = doppler.processSample(wetSample);

            // Write processed sample
            buffer.setSample(ch, sample, wetSample);
        }
    }

    // Apply dry/wet mix
    if (mix < 1.0f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);

            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * (1.0f - mix) + wet[i] * mix;
        }
    }

    // Apply output gain
    if (outputGainLinear != 1.0f)
        buffer.applyGain(0, numSamples, outputGainLinear);

    // Apply brickwall peak limiter
    if (limiterEnabled)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            float env = limiterEnvelope[ch];

            for (int i = 0; i < numSamples; ++i)
            {
                const float absSample = std::fabs(data[i]);

                // Envelope follower: fast attack, slow release
                if (absSample > env)
                    env = absSample + LIMITER_ATTACK_COEFF * (env - absSample);
                else
                    env = absSample + LIMITER_RELEASE_COEFF * (env - absSample);

                // Apply gain reduction if envelope exceeds ceiling
                if (env > LIMITER_CEILING)
                    data[i] *= LIMITER_CEILING / env;
            }

            limiterEnvelope[ch] = env;
        }
    }
}

void TransientEngine::reset()
{
    envelope.prepare(currentSampleRate);
    doppler.reset();
    sinePhase = 0.0f;
    limiterEnvelope[0] = 0.0f;
    limiterEnvelope[1] = 0.0f;

    for (int i = 0; i < PINK_NOISE_STAGES; ++i)
        pinkState[i] = 0.0f;
}

// ---------------------------------------------------------------------------
// Parameter setters
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
    intensity = percent * PERCENT_TO_FRACTION;
}

void TransientEngine::setPitchShift(float semitones)
{
    doppler.setPitchShiftSemitones(semitones);
}

void TransientEngine::setMix(float percent)
{
    mix = percent * PERCENT_TO_FRACTION;
}

void TransientEngine::setInputMode(InputMode mode)
{
    inputMode = mode;
}

void TransientEngine::setOutputGain(float dB)
{
    outputGainLinear = std::pow(10.0f, dB / 20.0f);
}

void TransientEngine::setLimiterEnabled(bool enabled)
{
    limiterEnabled = enabled;
}

// ---------------------------------------------------------------------------
// Internal source generators
// ---------------------------------------------------------------------------

float TransientEngine::generateInputSample(int /*channel*/)
{
    // For internal sources, generate a mono sample shared across channels
    switch (inputMode)
    {
        case InputMode::WhiteNoise:    return generateWhiteNoise();
        case InputMode::PinkNoise:     return generatePinkNoise();
        case InputMode::SineOscillator: return generateSine();
        case InputMode::ExternalAudio:
        default:                       return 0.0f;
    }
}

float TransientEngine::generateWhiteNoise()
{
    // Uniform random in [-1, 1]
    return noiseRng.nextFloat() * 2.0f - 1.0f;
}

float TransientEngine::generatePinkNoise()
{
    // Paul Kellet's refined method — 7 first-order filter stages
    // Attempt to approximate a -3dB/octave rolloff
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

    // Normalize to approximately [-1, 1]
    static constexpr float PINK_NOISE_GAIN = 0.11f;
    return pink * PINK_NOISE_GAIN;
}

float TransientEngine::generateSine()
{
    const float sample = std::sin(sinePhase * juce::MathConstants<float>::twoPi);
    sinePhase += sinePhaseInc;

    // Wrap phase to prevent floating-point drift
    if (sinePhase >= 1.0f)
        sinePhase -= 1.0f;

    return sample;
}
