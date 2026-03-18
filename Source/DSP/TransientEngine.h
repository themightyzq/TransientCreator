#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "EnvelopeGenerator.h"
#include "DopplerProcessor.h"

class TransientEngine
{
public:
    enum class InputMode
    {
        ExternalAudio = 0,
        WhiteNoise,
        PinkNoise,
        SineOscillator
    };

    TransientEngine();
    ~TransientEngine() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void processBlock(juce::AudioBuffer<float>& buffer, int numSamples);
    void reset();

    // Parameter setters (called per-block from PluginProcessor)
    void setTailLength(float ms);
    void setSilenceGap(float ms);
    void setShape(EnvelopeShape shape);
    void setIntensity(float percent);
    void setPitchShift(float semitones);
    void setMix(float percent);
    void setInputMode(InputMode mode);
    void setOutputGain(float dB);
    void setLimiterEnabled(bool enabled);

private:
    float generateInputSample(int channel);
    float generateWhiteNoise();
    float generatePinkNoise();
    float generateSine();

    // DSP constants
    static constexpr float SINE_FREQUENCY_HZ       = 440.0f;    // A4 for internal oscillator
    static constexpr float PERCENT_TO_FRACTION      = 0.01f;     // Convert 0-100% to 0.0-1.0
    static constexpr float LIMITER_CEILING          = 1.0f;      // 0 dBFS
    static constexpr float LIMITER_ATTACK_COEFF     = 0.9995f;   // ~0.1ms at 44.1k
    static constexpr float LIMITER_RELEASE_COEFF    = 0.9999f;   // ~50ms at 44.1k

    // State
    double currentSampleRate = 44100.0;
    int currentMaxBlockSize = 512;

    // Envelope
    EnvelopeGenerator envelope;

    // Doppler pitch-shift processor
    DopplerProcessor doppler;
    EnvelopeShape currentShape = EnvelopeShape::Exponential;
    float cachedTailLengthMs = 50.0f;

    // Parameters (cached per-block)
    float intensity = 0.75f;   // 0.0–1.0 (converted from percent)
    float mix       = 1.0f;    // 0.0–1.0 (converted from percent)
    float outputGainLinear = 1.0f;  // Linear gain from dB parameter
    bool limiterEnabled = true;
    InputMode inputMode = InputMode::ExternalAudio;

    // Limiter state (per-channel envelope followers)
    float limiterEnvelope[2] = { 0.0f, 0.0f };

    // Internal source generators
    juce::Random noiseRng { 42 };  // Pre-seeded for reproducibility

    // Pink noise state — Paul Kellet's refined method (7 first-order filters)
    static constexpr int PINK_NOISE_STAGES = 7;
    float pinkState[PINK_NOISE_STAGES] = {};

    // Sine oscillator
    float sinePhase     = 0.0f;
    float sinePhaseInc  = 0.0f;

    // Dry buffer for mix processing (pre-allocated)
    juce::AudioBuffer<float> dryBuffer;
};
