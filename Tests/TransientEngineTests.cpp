// TransientEngineTests.cpp
// Integration tests for the TransientEngine: finite/audible output, dry-signal
// passthrough at mix = 0, sample-rate independence of tail timing, and reset().

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

#include "DSP/TransientEngine.h"
#include "TestHelpers.h"

class TransientEngineTests : public juce::UnitTest
{
public:
    TransientEngineTests() : juce::UnitTest ("TransientEngine", "DSP") {}

    // Neutral starting configuration: 0 dB gains, full wet, no pitch/humanize.
    static void configure (TransientEngine& e, double sr, int block,
                           const float* lut, int lutSize)
    {
        e.prepare (sr, block);
        e.setCustomCurve (lut, lutSize);
        e.setShape (EnvelopeShape::Exponential);
        e.setAttackTime (0.0f);
        e.setSustainHold (0.0f);
        e.setTransientGain (0.0f);
        e.setOutputGain (0.0f);
        e.setMix (100.0f);
        e.setHumanize (0.0f);
        e.setPitchStart (0.0f);
        e.setPitchEnd (0.0f);
    }

    void runTest() override
    {
        const auto lut = TestHelpers::makeDecayLUT<512>();
        const int lutSize = static_cast<int> (lut.size());

        beginTest ("Sine input produces finite, audible, non-clipping output");
        {
            TransientEngine e;
            configure (e, 48000.0, 512, lut.data(), lutSize);
            e.setInputMode (TransientEngine::InputMode::SineOscillator);
            e.setSineFrequency (440.0f);
            e.setTailLength (50.0f);
            e.setSilenceGap (10.0f);

            juce::AudioBuffer<float> buf (2, 512);
            float peak = 0.0f;
            bool finite = true;
            for (int blk = 0; blk < 200; ++blk)   // ~2.1 s
            {
                buf.clear();
                e.processBlock (buf, 512);
                if (! TestHelpers::bufferIsFinite (buf, 512)) finite = false;
                peak = juce::jmax (peak, buf.getMagnitude (0, 512));
            }
            expect (finite, "engine produced a non-finite sample");
            expect (peak > 0.05f, "engine produced no audible output, peak = " + juce::String (peak));
            expect (peak <= 1.0f + 1.0e-3f, "engine exceeded unity at 0 dB, peak = " + juce::String (peak));
        }

        beginTest ("Mix = 0 passes the dry signal through unchanged");
        {
            TransientEngine e;
            configure (e, 48000.0, 256, lut.data(), lutSize);
            e.setInputMode (TransientEngine::InputMode::ExternalAudio);
            e.setMix (0.0f);

            auto fillConst = [] (juce::AudioBuffer<float>& b, float v)
            {
                for (int ch = 0; ch < b.getNumChannels(); ++ch)
                    for (int i = 0; i < b.getNumSamples(); ++i)
                        b.setSample (ch, i, v);
            };

            // Let the 20 ms mix smoother settle to its target (0 = fully dry).
            for (int blk = 0; blk < 10; ++blk)
            {
                juce::AudioBuffer<float> b (2, 256);
                fillConst (b, 0.5f);
                e.processBlock (b, 256);
            }

            juce::AudioBuffer<float> b (2, 256);
            fillConst (b, 0.5f);
            e.processBlock (b, 256);

            bool unchanged = true;
            for (int i = 0; i < 256; ++i)
                if (std::abs (b.getSample (0, i) - 0.5f) > 1.0e-3f)
                    unchanged = false;
            expect (unchanged, "dry signal was altered at mix = 0");
        }

        beginTest ("Tail duration scales with sample rate");
        {
            auto activeSamples = [&] (double sr)
            {
                TransientEngine e;
                configure (e, sr, 1, lut.data(), lutSize);
                e.setInputMode (TransientEngine::InputMode::SineOscillator);
                e.setTailLength (50.0f);
                e.setSilenceGap (10.0f);

                juce::AudioBuffer<float> b (2, 1);
                int guard = 0;
                while (! e.getIsInTail() && guard++ < static_cast<int> (sr))
                {
                    b.clear();
                    e.processBlock (b, 1);
                }
                int count = 0;
                while (e.getIsInTail() && count < static_cast<int> (sr))
                {
                    b.clear();
                    e.processBlock (b, 1);
                    ++count;
                }
                return count;
            };

            const int a44 = activeSamples (44100.0);
            const int a96 = activeSamples (96000.0);
            expect (a44 > 0 && a96 > 0, "no tail detected");

            const float ratio = static_cast<float> (a96) / static_cast<float> (a44);
            expectWithinAbsoluteError (ratio, 96000.0f / 44100.0f, 0.05f,
                                       "tail length is not sample-rate independent");
        }

        beginTest ("reset() returns the engine to silence");
        {
            TransientEngine e;
            configure (e, 48000.0, 64, lut.data(), lutSize);
            e.setInputMode (TransientEngine::InputMode::SineOscillator);
            e.setTailLength (50.0f);
            e.setSilenceGap (10.0f);

            juce::AudioBuffer<float> b (2, 64);
            for (int blk = 0; blk < 50; ++blk) { b.clear(); e.processBlock (b, 64); }

            e.reset();
            expect (! e.getIsInTail(), "engine should not be in-tail immediately after reset");
        }
    }
};

static TransientEngineTests transientEngineTests;
