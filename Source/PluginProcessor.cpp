#include "PluginProcessor.h"
#include "PluginEditor.h"

// Threshold set to -1.0dB to prevent inter-sample true-peak overs.
// juce::dsp::Limiter uses two compressor stages + hard clip at 0dBFS.
// A 0dB threshold defeats the first stage and allows inter-sample peaks
// that DAW true-peak metering (Reaper, Pro Tools) reports as overloads.
static constexpr float LIMITER_THRESHOLD_DB = -1.0f;
static constexpr float LIMITER_RELEASE_MS   = 50.0f;

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
    outputGainParam  = apvts.getRawParameterValue(ParamIDs::OUTPUT_GAIN);
    limiterOnParam   = apvts.getRawParameterValue(ParamIDs::LIMITER_ON);
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

    // Prepare juce::dsp::Limiter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    limiter.prepare(spec);
    limiter.setThreshold(LIMITER_THRESHOLD_DB);
    limiter.setRelease(LIMITER_RELEASE_MS);
}

void TransientCreatorProcessor::releaseResources()
{
    transientEngine.reset();
    limiter.reset();
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

    // Read discrete parameters once per block (no smoothing needed)
    const auto currentShape = static_cast<EnvelopeShape>(static_cast<int>(shapeParam->load()));
    const bool syncEnabled = syncEnabledParam->load() >= 0.5f;
    const int currentSyncNote = static_cast<int>(syncNoteParam->load());
    const auto currentInputMode = static_cast<TransientEngine::InputMode>(static_cast<int>(inputModeParam->load()));

    // Read continuous parameters directly from atomics — engine does per-sample smoothing
    const float tailLengthMs = tailLengthParam->load();
    float silenceGapMs = silenceGapParam->load();

    // Host tempo sync: override silence gap to align cycle to beat division
    if (syncEnabled)
    {
        if (auto* playHead = getPlayHead())
        {
            auto posInfo = playHead->getPosition();
            if (posInfo.hasValue() && posInfo->getBpm().hasValue())
            {
                const double bpm = *posInfo->getBpm();
                if (bpm > 0.0)
                {
                    static constexpr double noteMultipliers[] = {
                        4.0, 2.0, 1.0, 0.5, 0.25, 0.125,
                        2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0
                    };

                    const int noteIndex = juce::jlimit(0, 8, currentSyncNote);
                    const double beatDurationMs = 60000.0 / bpm;
                    const double noteDurationMs = beatDurationMs * noteMultipliers[noteIndex];

                    silenceGapMs = static_cast<float>(
                        std::max(0.0, noteDurationMs - static_cast<double>(tailLengthMs)));
                }
            }
        }
    }

    // Push parameters to engine — raw values, engine smooths internally
    transientEngine.setTailLength(tailLengthMs);
    transientEngine.setSilenceGap(silenceGapMs);
    transientEngine.setShape(currentShape);
    transientEngine.setIntensity(intensityParam->load());
    transientEngine.setPitchShift(pitchShiftParam->load());
    transientEngine.setMix(mixParam->load());
    transientEngine.setInputMode(currentInputMode);
    transientEngine.setOutputGain(outputGainParam->load());

    transientEngine.processBlock(buffer, buffer.getNumSamples());

    // juce::dsp::Limiter — absolute final stage in the output chain
    const bool limiterOn = limiterOnParam->load() >= 0.5f;
    if (limiterOn)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        limiter.process(context);
    }
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
