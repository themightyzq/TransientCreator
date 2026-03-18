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

    // Parameter setters (called per-block from PluginProcessor with raw atomic values)
    void setTailLength(float ms);
    void setSilenceGap(float ms);
    void setShape(EnvelopeShape shape);
    void setIntensity(float percent);
    void setPitchShift(float semitones);
    void setMix(float percent);
    void setInputMode(InputMode mode);
    void setOutputGain(float dB);

private:
    // Generate one mono sample from the internal source (called once per sample step)
    float generateSourceSample();
    float generateWhiteNoise();
    float generatePinkNoise();
    float generateSine();

    // DSP constants
    static constexpr float SINE_FREQUENCY_HZ       = 440.0f;    // A4 for internal oscillator
    static constexpr float PERCENT_TO_FRACTION      = 0.01f;     // Convert 0-100% to 0.0-1.0

    // State
    double currentSampleRate = 44100.0;
    int currentMaxBlockSize = 512;

    // Envelope
    EnvelopeGenerator envelope;

    // Doppler pitch-shift processor (stereo)
    DopplerProcessor doppler;
    EnvelopeShape currentShape = EnvelopeShape::Exponential;
    float cachedTailLengthMs = 50.0f;

    // Per-sample smoothed parameters (Fix 1D: smoothing lives in the engine)
    juce::SmoothedValue<float> intensitySmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;  // Smoothed in dB, converted per-sample

    // Discrete parameter (no smoothing)
    InputMode inputMode = InputMode::ExternalAudio;

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
