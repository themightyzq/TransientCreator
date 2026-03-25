#pragma once

#include <array>
#include <atomic>
#include <cmath>
#include <vector>
#include <algorithm>

struct SharedUIState
{
    static constexpr int CUSTOM_CURVE_SIZE = 512;

    // UI -> Audio: custom curve LUT data (lock-free via atomic flag)
    std::array<float, CUSTOM_CURVE_SIZE> customCurveStaging {};
    std::atomic<bool> customCurveUpdated { false };

    // Audio -> UI: playhead position (0-1 over full tail+gap cycle)
    std::atomic<float> playheadPosition { 0.0f };
    std::atomic<bool> isInTail { false };

    // Audio -> UI: push loaded curve back to display after state restore
    std::array<float, CUSTOM_CURVE_SIZE> customCurveDisplay {};
    std::atomic<bool> customCurveLoaded { false };

    // Breakpoints (message thread only — UI editing + state serialization)
    struct Breakpoint
    {
        float x;        // 0-1 normalized position in decay
        float y;        // 0-1 amplitude
        float tension;  // -1 to +1: negative = concave, positive = convex, 0 = linear

        bool operator<(const Breakpoint& other) const { return x < other.x; }
    };

    std::vector<Breakpoint> breakpoints;

    // Regenerate the LUT from breakpoints via linear interpolation with per-segment tension warp
    void rebuildLUTFromBreakpoints()
    {
        if (breakpoints.size() < 2) return;

        std::sort(breakpoints.begin(), breakpoints.end());

        const size_t numBP = breakpoints.size();

        for (size_t i = 0; i < static_cast<size_t>(CUSTOM_CURVE_SIZE); ++i)
        {
            const float normPos = static_cast<float>(i) / static_cast<float>(CUSTOM_CURVE_SIZE - 1);

            // Find the segment [left, right] containing normPos
            size_t right = 1;
            for (; right < numBP; ++right)
            {
                if (breakpoints[right].x >= normPos) break;
            }
            if (right >= numBP) right = numBP - 1;
            const size_t left = (right > 0) ? right - 1 : 0;

            if (left == right)
            {
                customCurveStaging[i] = breakpoints[left].y;
                continue;
            }

            // Linear interpolation parameter
            const float span = breakpoints[right].x - breakpoints[left].x;
            float t = (span > 0.0f) ? (normPos - breakpoints[left].x) / span : 0.0f;
            t = std::clamp(t, 0.0f, 1.0f);

            // Apply segment tension (power curve warp on linear t)
            const float tension = breakpoints[left].tension;
            if (std::fabs(tension) > 0.01f)
            {
                if (tension > 0.0f)
                    t = std::pow(t, 1.0f / (1.0f + tension * 3.0f));  // convex
                else
                    t = std::pow(t, 1.0f + std::fabs(tension) * 3.0f);  // concave
            }

            // Linear interpolation with the (possibly warped) t
            const float value = breakpoints[left].y * (1.0f - t) + breakpoints[right].y * t;
            customCurveStaging[i] = std::clamp(value, 0.0f, 1.0f);
        }
    }

    SharedUIState()
    {
        initDefaultCurve();
    }

    void initDefaultCurve()
    {
        breakpoints.clear();
        breakpoints.push_back({ 0.00f, 1.00f,  0.7f });
        breakpoints.push_back({ 1.00f, 0.03f,  0.0f });

        rebuildLUTFromBreakpoints();
        std::copy(customCurveStaging.begin(), customCurveStaging.end(), customCurveDisplay.begin());
    }
};
