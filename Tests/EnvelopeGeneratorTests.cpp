// EnvelopeGeneratorTests.cpp
// Unit tests for EnvelopeGenerator: tail-duration accuracy across sample rates,
// amplitude range, attack ramp, decay monotonicity, silence-gap behaviour, and
// the static visualization method's region boundaries.

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

#include "DSP/EnvelopeGenerator.h"
#include "DSP/EnvelopeConstants.h"
#include "TestHelpers.h"

class EnvelopeGeneratorTests : public juce::UnitTest
{
public:
    EnvelopeGeneratorTests() : juce::UnitTest ("EnvelopeGenerator", "DSP") {}

    void runTest() override
    {
        using namespace EnvelopeConstants;
        const auto lut = TestHelpers::makeDecayLUT<512>();
        const int lutSize = static_cast<int> (lut.size());

        beginTest ("Tail duration matches sampleRate x tailLength");
        {
            for (double sr : { 44100.0, 48000.0, 96000.0 })
            {
                EnvelopeGenerator env;
                env.prepare (sr);
                env.setCustomCurve (lut.data(), lutSize);
                env.setAttackTime (0.0f);
                env.setSustainHold (0.0f);
                env.setSilenceGap (1000.0f);   // long gap: no auto-retrigger during the count
                env.setTailLength (100.0f);

                const int expected = juce::jmax (1, static_cast<int> ((100.0f / 1000.0f) * static_cast<float> (sr)));

                env.trigger();
                int inTail = 0;
                for (int i = 0; i < expected * 2 + 16; ++i)
                {
                    env.getNextSample();
                    if (env.isInTail()) ++inTail;
                    else break;
                }
                expectEquals (inTail, expected, "sampleRate = " + juce::String (sr));
            }
        }

        beginTest ("Output stays within [0, 1] over many cycles");
        {
            EnvelopeGenerator env;
            env.prepare (48000.0);
            env.setCustomCurve (lut.data(), lutSize);
            env.setTailLength (20.0f);
            env.setSilenceGap (10.0f);
            env.setAttackTime (2.0f);
            env.setSustainHold (10.0f);
            env.trigger();

            bool inRange = true;
            bool allFinite = true;
            for (int i = 0; i < 48000; ++i)   // ~1 s, ~33 cycles
            {
                const float v = env.getNextSample();
                if (! std::isfinite (v)) allFinite = false;
                if (v < -1.0e-4f || v > 1.0f + 1.0e-4f) inRange = false;
            }
            expect (allFinite, "envelope produced a non-finite sample");
            expect (inRange, "envelope left the [0, 1] range");
        }

        beginTest ("Attack ramps up from ~0 toward peak");
        {
            EnvelopeGenerator env;
            env.prepare (48000.0);
            env.setCustomCurve (lut.data(), lutSize);
            env.setTailLength (50.0f);
            env.setSilenceGap (1000.0f);
            env.setSustainHold (0.0f);
            env.setAttackTime (5.0f);   // 240 samples at 48 kHz

            env.trigger();
            const int attackSamps = static_cast<int> ((5.0f / 1000.0f) * 48000.0f);
            const float first = env.getNextSample();
            float prev = first;
            bool nonDecreasing = true;
            for (int i = 1; i < attackSamps - 1; ++i)
            {
                const float v = env.getNextSample();
                if (v < prev - 1.0e-4f) nonDecreasing = false;
                prev = v;
            }
            expect (first < 0.05f, "attack should start near 0, got " + juce::String (first));
            expect (nonDecreasing, "attack ramp should be non-decreasing");
            expect (prev > 0.8f, "attack should approach the peak, reached " + juce::String (prev));
        }

        beginTest ("Decay is non-increasing for a decreasing LUT");
        {
            EnvelopeGenerator env;
            env.prepare (48000.0);
            env.setCustomCurve (lut.data(), lutSize);
            env.setTailLength (200.0f);
            env.setSilenceGap (1000.0f);
            env.setAttackTime (0.0f);
            env.setSustainHold (0.0f);

            env.trigger();
            // Skip the short raised-cosine onset fade (applied when attack == 0).
            for (int i = 0; i < MIN_ONSET_FADE_SAMPLES + 1; ++i) env.getNextSample();

            float prev = env.getNextSample();
            bool nonIncreasing = true;
            while (env.isInTail())
            {
                const float v = env.getNextSample();
                if (! env.isInTail()) break;   // ignore the trailing transition-to-silence sample
                if (v > prev + 1.0e-4f) { nonIncreasing = false; break; }
                prev = v;
            }
            expect (nonIncreasing, "decay phase should be non-increasing");
        }

        beginTest ("Silence gap is zero-valued and auto-retriggers");
        {
            EnvelopeGenerator env;
            env.prepare (48000.0);
            env.setCustomCurve (lut.data(), lutSize);
            env.setAttackTime (0.0f);
            env.setSustainHold (0.0f);
            env.setTailLength (20.0f);
            env.setSilenceGap (20.0f);

            env.trigger();
            while (env.isInTail()) env.getNextSample();   // run out the tail

            const int gapSamps = static_cast<int> ((20.0f / 1000.0f) * 48000.0f);
            bool allZero = true;
            for (int i = 0; i < gapSamps - 2; ++i)
            {
                const float v = env.getNextSample();
                if (v != 0.0f || env.isInTail()) allZero = false;
            }
            expect (allZero, "gap should be exactly zero and not in-tail");

            bool retriggered = false;
            for (int i = 0; i < gapSamps; ++i)
            {
                env.getNextSample();
                if (env.isInTail()) { retriggered = true; break; }
            }
            expect (retriggered, "envelope should auto-retrigger after the gap");
        }

        beginTest ("computeShapeAtNormalized: attack / hold / decay regions");
        {
            const float atk = 0.2f;
            const float hold = 0.2f;

            const float atStart = EnvelopeGenerator::computeShapeAtNormalized (
                0.0f, EnvelopeShape::Exponential, atk, hold, lut.data(), lutSize);
            expect (atStart < 1.0e-4f, "attack should start at 0");

            const float endOfAttack = EnvelopeGenerator::computeShapeAtNormalized (
                atk * 0.999f, EnvelopeShape::Exponential, atk, hold, lut.data(), lutSize);
            expectWithinAbsoluteError (endOfAttack, 1.0f, 0.01f, "attack should reach the peak");

            const float inHold = EnvelopeGenerator::computeShapeAtNormalized (
                atk + hold * 0.5f, EnvelopeShape::Exponential, atk, hold, lut.data(), lutSize);
            expectEquals (inHold, 1.0f, "hold region should be flat at 1.0");

            const float atEnd = EnvelopeGenerator::computeShapeAtNormalized (
                1.0f, EnvelopeShape::Exponential, atk, hold, lut.data(), lutSize);
            expect (atEnd < 0.05f, "decay should approach 0 at the end");

            const float noLut = EnvelopeGenerator::computeShapeAtNormalized (
                0.9f, EnvelopeShape::Exponential, 0.0f, 0.0f, nullptr, 0);
            expectEquals (noLut, 0.0f, "decay without a LUT should be 0");
        }
    }
};

static EnvelopeGeneratorTests envelopeGeneratorTests;
