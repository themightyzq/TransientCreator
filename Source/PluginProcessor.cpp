#include "PluginProcessor.h"
#include "PluginEditor.h"

TransientCreatorProcessor::TransientCreatorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
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
