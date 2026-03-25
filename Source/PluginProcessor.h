#pragma once

#include <JuceHeader.h>
#include "Parameters/ParameterLayout.h"
#include "DSP/TransientEngine.h"
#include "SharedState.h"

class TransientCreatorProcessor : public juce::AudioProcessor
{
public:
    TransientCreatorProcessor();
    ~TransientCreatorProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Lock-free shared state for UI communication
    SharedUIState sharedState;

    // Audio-thread custom curve LUT (copied from staging at block boundary)
    std::array<float, SharedUIState::CUSTOM_CURVE_SIZE> customCurveLUT {};

private:
    // Caches raw parameter pointers for audio-thread-safe reads
    std::atomic<float>* tailLengthParam      = nullptr;
    std::atomic<float>* silenceGapParam      = nullptr;
    std::atomic<float>* mixParam             = nullptr;
    std::atomic<float>* shapeParam           = nullptr;
    std::atomic<float>* syncEnabledParam     = nullptr;
    std::atomic<float>* syncNoteParam        = nullptr;
    std::atomic<float>* inputModeParam       = nullptr;
    std::atomic<float>* outputGainParam      = nullptr;
    std::atomic<float>* limiterOnParam       = nullptr;
    std::atomic<float>* attackTimeParam      = nullptr;
    std::atomic<float>* transientGainParam   = nullptr;
    std::atomic<float>* sineFreqParam        = nullptr;
    std::atomic<float>* pitchStartParam      = nullptr;
    std::atomic<float>* pitchEndParam        = nullptr;
    std::atomic<float>* humanizeParam        = nullptr;
    std::atomic<float>* sustainHoldParam     = nullptr;

    TransientEngine transientEngine;

    // Output limiter
    juce::dsp::Compressor<float> limiterCompressor;
    static constexpr float HARD_CEILING_LINEAR = 0.9441f;  // -0.5 dBFS

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientCreatorProcessor)
};
