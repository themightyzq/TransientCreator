#pragma once

#include <JuceHeader.h>
#include "Parameters/ParameterLayout.h"
#include "DSP/TransientEngine.h"

class TransientCreatorProcessor : public juce::AudioProcessor
{
public:
    TransientCreatorProcessor();
    ~TransientCreatorProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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

private:
    // Caches raw parameter pointers for audio-thread-safe reads (no locking)
    std::atomic<float>* tailLengthParam  = nullptr;
    std::atomic<float>* silenceGapParam  = nullptr;
    std::atomic<float>* intensityParam   = nullptr;
    std::atomic<float>* pitchShiftParam  = nullptr;
    std::atomic<float>* mixParam         = nullptr;
    std::atomic<float>* shapeParam       = nullptr;
    std::atomic<float>* syncEnabledParam = nullptr;
    std::atomic<float>* syncNoteParam    = nullptr;
    std::atomic<float>* inputModeParam   = nullptr;
    std::atomic<float>* outputGainParam  = nullptr;
    std::atomic<float>* limiterOnParam   = nullptr;

    // SmoothedValue wrappers for continuously-modulated parameters
    juce::SmoothedValue<float> tailLengthSmoothed;
    juce::SmoothedValue<float> silenceGapSmoothed;
    juce::SmoothedValue<float> intensitySmoothed;
    juce::SmoothedValue<float> pitchShiftSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    juce::SmoothedValue<float> outputGainSmoothed;

    TransientEngine transientEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransientCreatorProcessor)
};
