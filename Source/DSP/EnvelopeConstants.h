#pragma once

namespace EnvelopeConstants
{
    inline constexpr float ENVELOPE_THRESHOLD     = 0.001f;    // -60dB floor
    inline constexpr float LOG_CURVATURE_K        = 10.0f;     // Logarithmic shape curvature
    inline constexpr float GAUSSIAN_SIGMA_RATIO   = 0.25f;     // sigma = T/4
    inline constexpr float DOUBLE_TAP_SPACING     = 0.3f;      // Second tap at 30% of tail
    inline constexpr float PERCUSSIVE_ATTACK_S    = 0.001f;    // 1ms absolute attack
    inline constexpr float PERCUSSIVE_BODY_RATIO  = 0.15f;     // 15% of tail is body
    inline constexpr float PERCUSSIVE_BODY_DROP   = 0.3f;      // Body drops to 0.7 amplitude
    inline constexpr int   CROSSFADE_SAMPLES      = 32;        // Anti-click crossfade length
} // namespace EnvelopeConstants
