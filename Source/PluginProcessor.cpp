#include "PluginProcessor.h"
#include "PluginEditor.h"

TransientCreatorProcessor::TransientCreatorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    // Cache raw parameter pointers — these are lock-free atomics safe for the audio thread
    tailLengthParam  = apvts.getRawParameterValue(ParamIDs::TAIL_LENGTH);
    silenceGapParam  = apvts.getRawParameterValue(ParamIDs::SILENCE_GAP);
    intensityParam   = apvts.getRawParameterValue(ParamIDs::INTENSITY);
    pitchShiftParam  = apvts.getRawParameterValue(ParamIDs::PITCH_SHIFT);
    mixParam         = apvts.getRawParameterValue(ParamIDs::MIX);
    shapeParam       = apvts.getRawParameterValue(ParamIDs::SHAPE);
    syncEnabledParam = apvts.getRawParameterValue(ParamIDs::SYNC_ENABLED);
    syncNoteParam    = apvts.getRawParameterValue(ParamIDs::SYNC_NOTE);
    inputModeParam   = apvts.getRawParameterValue(ParamIDs::INPUT_MODE);
}

TransientCreatorProcessor::~TransientCreatorProcessor() = default;

const juce::String TransientCreatorProcessor::getName() const { return JucePlugin_Name; }
bool TransientCreatorProcessor::acceptsMidi() const { return false; }
bool TransientCreatorProcessor::producesMidi() const { return false; }
bool TransientCreatorProcessor::isMidiEffect() const { return false; }
double TransientCreatorProcessor::getTailLengthSeconds() const { return 0.0; }
int TransientCreatorProcessor::getNumPrograms() { return 1; }
int TransientCreatorProcessor::getCurrentProgram() { return 0; }
void TransientCreatorProcessor::setCurrentProgram(int) {}
const juce::String TransientCreatorProcessor::getProgramName(int) { return {}; }
void TransientCreatorProcessor::changeProgramName(int, const juce::String&) {}

void TransientCreatorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Initialize SmoothedValues with current parameter values and ramp time
    tailLengthSmoothed.reset(sampleRate, ParamDefaults::SMOOTHING_RAMP_SEC);
    tailLengthSmoothed.setCurrentAndTargetValue(tailLengthParam->load());

    silenceGapSmoothed.reset(sampleRate, ParamDefaults::SMOOTHING_RAMP_SEC);
    silenceGapSmoothed.setCurrentAndTargetValue(silenceGapParam->load());

    intensitySmoothed.reset(sampleRate, ParamDefaults::SMOOTHING_RAMP_SEC);
    intensitySmoothed.setCurrentAndTargetValue(intensityParam->load());

    pitchShiftSmoothed.reset(sampleRate, ParamDefaults::SMOOTHING_RAMP_SEC);
    pitchShiftSmoothed.setCurrentAndTargetValue(pitchShiftParam->load());

    mixSmoothed.reset(sampleRate, ParamDefaults::SMOOTHING_RAMP_SEC);
    mixSmoothed.setCurrentAndTargetValue(mixParam->load());

    transientEngine.prepare(sampleRate, samplesPerBlock);
}

void TransientCreatorProcessor::releaseResources()
{
    transientEngine.reset();
}

bool TransientCreatorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void TransientCreatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update smoothed parameter targets from atomic caches
    tailLengthSmoothed.setTargetValue(tailLengthParam->load());
    silenceGapSmoothed.setTargetValue(silenceGapParam->load());
    intensitySmoothed.setTargetValue(intensityParam->load());
    pitchShiftSmoothed.setTargetValue(pitchShiftParam->load());
    mixSmoothed.setTargetValue(mixParam->load());

    // Discrete parameters — read once per block (no smoothing needed)
    // These will be passed to TransientEngine in Phase 3
    // const int currentShape     = static_cast<int>(shapeParam->load());
    // const bool syncEnabled     = syncEnabledParam->load() >= 0.5f;
    // const int currentSyncNote  = static_cast<int>(syncNoteParam->load());
    // const int currentInputMode = static_cast<int>(inputModeParam->load());

    // Stub: passthrough — TransientEngine processing added in Phase 3
    transientEngine.processBlock(buffer, buffer.getNumSamples());
}

void TransientCreatorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TransientCreatorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorEditor* TransientCreatorProcessor::createEditor()
{
    return new TransientCreatorEditor(*this);
}

bool TransientCreatorProcessor::hasEditor() const { return true; }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TransientCreatorProcessor();
}
