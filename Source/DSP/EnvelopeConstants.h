#pragma once

namespace EnvelopeConstants
{
    inline constexpr float ENVELOPE_THRESHOLD       = 0.001f;    // -60dB floor
    inline constexpr float LOG_POWER_CURVATURE      = 3.0f;      // Power curve for Logarithmic shape
    inline constexpr float DOUBLE_TAP_SPACING       = 0.18f;     // Second tap at 18% of decay
    inline constexpr float DOUBLE_TAP_THRESHOLD     = 0.01f;     // -40dB per-tap floor
    inline constexpr float DOUBLE_TAP_DECAY_SPAN    = 0.6f;      // Each tap decays over 60% of D
    inline constexpr float PERCUSSIVE_ATTACK_S      = 0.001f;    // 1ms absolute attack
    inline constexpr float PERCUSSIVE_BODY_RATIO    = 0.15f;     // 15% of tail is body
    inline constexpr float PERCUSSIVE_BODY_DROP     = 0.3f;      // Body drops to 0.7 amplitude
    inline constexpr float REVERSE_SAW_HOLD_FRACTION = 0.12f;    // Hold at 1.0 for first 12% of decay
    inline constexpr int   CROSSFADE_SAMPLES        = 32;        // Anti-click crossfade length
    inline constexpr int   MIN_ONSET_FADE_SAMPLES   = 4;         // Minimum silence-to-tail smoothing
} // namespace EnvelopeConstants
