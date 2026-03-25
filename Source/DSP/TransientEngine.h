#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "EnvelopeGenerator.h"
#include "DopplerProcessor.h"
#include "../SharedState.h"

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
    void setPitchStart(float semitones);
    void setPitchEnd(float semitones);
    void setMix(float percent);
    void setInputMode(InputMode mode);
    void setOutputGain(float dB);
    void setAttackTime(float ms);
    void setTransientGain(float dB);
    void setSineFrequency(float hz);
    void setHumanize(float percent);
    void setSustainHold(float percent);

    // Custom curve: set pointer to the LUT (owned by caller)
    void setCustomCurve(const float* lut, int size);

    // Playhead: returns normalized cycle position and tail state
    float getPlayheadPosition() const;
    bool getIsInTail() const;

private:
    void generateSourceSampleStereo(float& outL, float& outR);
    float generateWhiteNoise();
    float generatePinkNoiseSample();
    float generateSine();

    static constexpr float PERCENT_TO_FRACTION = 0.01f;
    static constexpr int   NUM_CHANNELS        = 2;
    double currentSampleRate = 44100.0;
    int currentMaxBlockSize = 512;

    // Envelope
    EnvelopeGenerator envelope;

    // Doppler (independent of shape — active when pitch > 0)
    DopplerProcessor doppler;
    float cachedTailLengthMs = 150.0f;

    // Per-sample smoothed parameters
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;
    juce::SmoothedValue<float> transientGainSmoothed;

    // Discrete parameter
    InputMode inputMode = InputMode::ExternalAudio;

    // Internal source generators (stereo: independent L/R for noise)
    juce::Random noiseRngL { 42 };
    juce::Random noiseRngR { 137 };
    static constexpr int PINK_NOISE_STAGES = 7;
    float pinkStateL[PINK_NOISE_STAGES] = {};
    float pinkStateR[PINK_NOISE_STAGES] = {};
    float sinePhase    = 0.0f;
    float sinePhaseInc = 0.0f;

    // Dry buffer for mix processing
    juce::AudioBuffer<float> dryBuffer;
    // Wet buffer for filter processing
    juce::AudioBuffer<float> wetBuffer;
};
