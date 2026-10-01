#pragma once

#include <array>
#include <cmath>
#include <juce_audio_basics/juce_audio_basics.h>

// Shared utilities for the DSP unit tests.
namespace TestHelpers
{
    // Strictly-decreasing exponential decay LUT in [0, 1].
    // lut[0] == 1.0 (peak), lut[N-1] == exp(-6) ≈ 0.0025 (below the -60 dB floor).
    // The engine reads its decay phase entirely from such a LUT, so tests must
    // supply one — the EnvelopeShape enum only seeds the UI-built curve.
    template <int N>
    inline std::array<float, N> makeDecayLUT()
    {
        static_assert (N >= 2, "LUT needs at least two points");
        std::array<float, N> lut {};
        for (int i = 0; i < N; ++i)
        {
            const float t = static_cast<float> (i) / static_cast<float> (N - 1);
            lut[static_cast<size_t> (i)] = std::exp (-6.0f * t);
        }
        return lut;
    }

    inline bool bufferIsFinite (const juce::AudioBuffer<float>& buffer, int numSamples)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < numSamples; ++i)
                if (! std::isfinite (buffer.getSample (ch, i)))
                    return false;
        return true;
    }
} // namespace TestHelpers
