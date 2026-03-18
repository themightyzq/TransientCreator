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
    // Phase 4 new setters
    void setAttackTime(float ms);
    void setTransientGain(float dB);
    void setTension(float t);
    void setHPFFrequency(float hz);
    void setLPFFrequency(float hz);
    void setSineFrequency(float hz);
    void setDopplerDirection(DopplerProcessor::Direction dir);
    void setPreDelay(float ms);
    void setHumanize(float percent);

private:
    float generateSourceSample();
    float generateWhiteNoise();
    float generatePinkNoise();
    float generateSine();

    static constexpr float PERCENT_TO_FRACTION = 0.01f;
    static constexpr int   NUM_CHANNELS        = 2;
    static constexpr float PRE_DELAY_MAX_MS    = 50.0f;

    double currentSampleRate = 44100.0;
    int currentMaxBlockSize = 512;

    // Envelope
    EnvelopeGenerator envelope;

    // Doppler
    DopplerProcessor doppler;
    EnvelopeShape currentShape = EnvelopeShape::Exponential;
    float cachedTailLengthMs = 50.0f;

    // Per-sample smoothed parameters
    juce::SmoothedValue<float> intensitySmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> transientGainSmoothed;

    // Discrete parameter
    InputMode inputMode = InputMode::ExternalAudio;

    // Internal source generators
    juce::Random noiseRng { 42 };
    static constexpr int PINK_NOISE_STAGES = 7;
    float pinkState[PINK_NOISE_STAGES] = {};
    float sinePhase    = 0.0f;
    float sinePhaseInc = 0.0f;

    // Output filters — 4D
    juce::dsp::StateVariableTPTFilter<float> highPassFilter;
    juce::dsp::StateVariableTPTFilter<float> lowPassFilter;

    // Pre-delay — 4G
    std::vector<float> preDelayBuffer[NUM_CHANNELS];
    int preDelayWriteIndex = 0;
    int preDelaySamples    = 0;
    int preDelayBufferSize = 0;
    int preDelayBufferMask = 0;

    // Dry buffer for mix processing
    juce::AudioBuffer<float> dryBuffer;
    // Wet buffer for filter processing
    juce::AudioBuffer<float> wetBuffer;
};
