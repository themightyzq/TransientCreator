#include "PluginProcessor.h"
#include "PluginEditor.h"

// Limiter compressor settings
static constexpr float LIMITER_THRESHOLD_DB = -6.0f;   // Start compressing at -6dBFS
static constexpr float LIMITER_RATIO        = 100.0f;   // Near-infinite ratio
static constexpr float LIMITER_ATTACK_MS    = 0.01f;    // 10 microsecond attack
static constexpr float LIMITER_RELEASE_MS   = 50.0f;    // 50ms release

TransientCreatorProcessor::TransientCreatorProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
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
double TransientCreatorProcessor::getTailLengthSeconds() const
{
    // Worst case: max tail + max gap so DAWs don't truncate during offline bounce
    return static_cast<double>(ParamDefaults::TAIL_LENGTH_MAX + ParamDefaults::SILENCE_GAP_MAX) / 1000.0;
}
int TransientCreatorProcessor::getNumPrograms() { return 1; }
int TransientCreatorProcessor::getCurrentProgram() { return 0; }
void TransientCreatorProcessor::setCurrentProgram(int) {}
const juce::String TransientCreatorProcessor::getProgramName(int) { return {}; }
void TransientCreatorProcessor::changeProgramName(int, const juce::String&) {}

void TransientCreatorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    transientEngine.prepare(sampleRate, samplesPerBlock);

    // Configure juce::dsp::Compressor as a fast limiter (no makeup gain, no opaque internals)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    limiterCompressor.prepare(spec);
    limiterCompressor.setThreshold(LIMITER_THRESHOLD_DB);
    limiterCompressor.setRatio(LIMITER_RATIO);
    limiterCompressor.setAttack(LIMITER_ATTACK_MS);
    limiterCompressor.setRelease(LIMITER_RELEASE_MS);
}

void TransientCreatorProcessor::releaseResources()
{
    transientEngine.reset();
    limiterCompressor.reset();
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

    // Read discrete parameters once per block
    const auto currentShape = static_cast<EnvelopeShape>(static_cast<int>(shapeParam->load()));
    const bool syncEnabled = syncEnabledParam->load() >= 0.5f;
    const int currentSyncNote = static_cast<int>(syncNoteParam->load());
    const auto currentInputMode = static_cast<TransientEngine::InputMode>(static_cast<int>(inputModeParam->load()));

    const float tailLengthMs = tailLengthParam->load();
    float silenceGapMs = silenceGapParam->load();

    // Host tempo sync
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

    // Push parameters to engine
    transientEngine.setTailLength(tailLengthMs);
    transientEngine.setSilenceGap(silenceGapMs);
    transientEngine.setShape(currentShape);
    transientEngine.setIntensity(intensityParam->load());
    transientEngine.setPitchShift(pitchShiftParam->load());
    transientEngine.setMix(mixParam->load());
    transientEngine.setInputMode(currentInputMode);
    transientEngine.setOutputGain(outputGainParam->load());

    transientEngine.processBlock(buffer, buffer.getNumSamples());

    // Output limiter — absolute final stage
    const bool limiterOn = limiterOnParam->load() >= 0.5f;
    if (limiterOn)
    {
        // Stage 1: Fast compressor configured as limiter (dynamics shaping)
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        limiterCompressor.process(context);

        // Stage 2: Hard ceiling clamp at -0.3dBFS — absolute safety net.
        // No sample can leave this plugin above this value, period.
        // This catches anything the compressor's attack time didn't fully contain.
        const int numSamples = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            juce::FloatVectorOperations::clip(
                buffer.getWritePointer(ch),
                buffer.getReadPointer(ch),
                -HARD_CEILING_LINEAR,
                HARD_CEILING_LINEAR,
                numSamples);
        }
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
