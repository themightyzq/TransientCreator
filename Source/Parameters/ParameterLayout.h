#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline constexpr const char* TAIL_LENGTH    = "tailLength";
    inline constexpr const char* SILENCE_GAP    = "silenceGap";
    inline constexpr const char* SHAPE          = "transientShape";
    inline constexpr const char* INTENSITY      = "intensity";
    inline constexpr const char* PITCH_SHIFT    = "pitchShift";
    inline constexpr const char* MIX            = "mix";
    inline constexpr const char* SYNC_ENABLED   = "syncEnabled";
    inline constexpr const char* SYNC_NOTE      = "syncNote";
    inline constexpr const char* INPUT_MODE     = "inputMode";
} // namespace ParamIDs

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
