#include "EnvelopeVisualizer.h"
#include <cmath>

EnvelopeVisualizer::EnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts)
{
    shapeParam      = apvts.getRawParameterValue(ParamIDs::SHAPE);
    tailLengthParam = apvts.getRawParameterValue(ParamIDs::TAIL_LENGTH);
    silenceGapParam = apvts.getRawParameterValue(ParamIDs::SILENCE_GAP);

    startTimerHz(REPAINT_HZ);
}

EnvelopeVisualizer::~EnvelopeVisualizer()
{
    stopTimer();
}

void EnvelopeVisualizer::timerCallback()
{
    repaint();
}

void EnvelopeVisualizer::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Background
    g.setColour(juce::Colour(0xff0a0e1a));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Read parameters
    const auto shape = static_cast<EnvelopeShape>(static_cast<int>(shapeParam->load()));
    const float tailMs = tailLengthParam->load();
    const float gapMs  = silenceGapParam->load();

    // Calculate proportions for tail vs gap display
    const float totalMs = tailMs + gapMs;
    const float tailFraction = (totalMs > 0.0f) ? (tailMs / totalMs) : TAIL_FRACTION;
    const float tailWidth = w * tailFraction;
    const float gapWidth  = w - tailWidth;

    // Draw gap region (dimmed)
    if (gapWidth > 0.0f)
    {
        g.setColour(juce::Colour(0xff111827));
        g.fillRoundedRectangle(bounds.getX() + tailWidth, bounds.getY(), gapWidth, h, 0.0f);
    }

    // Draw separator line between tail and gap
    g.setColour(juce::Colour(0xff334155));
    g.drawVerticalLine(static_cast<int>(bounds.getX() + tailWidth), bounds.getY(), bounds.getBottom());

    // Build envelope path
    const float tailSamples = 1000.0f; // Normalized reference for shape computation
    juce::Path envelopePath;
    bool pathStarted = false;

    for (int i = 0; i <= RESOLUTION; ++i)
    {
        const float normalizedX = static_cast<float>(i) / static_cast<float>(RESOLUTION);
        float amplitude = 0.0f;

        if (normalizedX <= tailFraction && tailFraction > 0.0f)
        {
            // Within tail region
            const float tailNorm = normalizedX / tailFraction;
            amplitude = computeEnvelopeAt(tailNorm, shape, tailSamples);
        }
        // else: in gap region, amplitude = 0

        const float px = bounds.getX() + normalizedX * w;
        const float py = bounds.getBottom() - amplitude * (h - 8.0f) - 4.0f;

        if (!pathStarted)
        {
            envelopePath.startNewSubPath(px, py);
            pathStarted = true;
        }
        else
        {
            envelopePath.lineTo(px, py);
        }
    }

    // Draw envelope curve
    g.setColour(juce::Colour(0xff3b82f6));
    g.strokePath(envelopePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

    // Fill under curve
    juce::Path fillPath(envelopePath);
    fillPath.lineTo(bounds.getRight(), bounds.getBottom());
    fillPath.lineTo(bounds.getX(), bounds.getBottom());
    fillPath.closeSubPath();
    g.setColour(juce::Colour(0xff3b82f6).withAlpha(0.15f));
    g.fillPath(fillPath);

    // Labels
    g.setColour(juce::Colour(0xff94a3b8));
    g.setFont(juce::FontOptions(11.0f));

    const juce::String tailLabel = juce::String(tailMs, 1) + " ms";
    const juce::String gapLabel  = juce::String(gapMs, 1) + " ms";

    if (tailWidth > 40.0f)
        g.drawText(tailLabel, juce::Rectangle<float>(bounds.getX(), bounds.getBottom() - 16.0f, tailWidth, 14.0f),
                   juce::Justification::centred);
    if (gapWidth > 40.0f)
        g.drawText(gapLabel, juce::Rectangle<float>(bounds.getX() + tailWidth, bounds.getBottom() - 16.0f, gapWidth, 14.0f),
                   juce::Justification::centred);
}

void EnvelopeVisualizer::resized()
{
    // No child layout needed
}

float EnvelopeVisualizer::computeEnvelopeAt(float normalizedPos, EnvelopeShape shape, float tailSamples) const
{
    const float t = normalizedPos * tailSamples;
    const float T = tailSamples;

    switch (shape)
    {
        case EnvelopeShape::Exponential:
        case EnvelopeShape::Doppler:
        {
            const float decayRate = -std::log(ENVELOPE_THRESHOLD) / T;
            return std::exp(-t * decayRate);
        }

        case EnvelopeShape::Linear:
            return 1.0f - (t / T);

        case EnvelopeShape::Logarithmic:
        {
            const float denom = std::log(1.0f + T * LOG_CURVATURE_K);
            return 1.0f - std::log(1.0f + t * LOG_CURVATURE_K) / denom;
        }

        case EnvelopeShape::ReverseSawtooth:
            return (t < T) ? (1.0f - t / T) : 0.0f;

        case EnvelopeShape::Gaussian:
        {
            const float sigma = T * GAUSSIAN_SIGMA_RATIO;
            const float ratio = t / sigma;
            return std::exp(-0.5f * ratio * ratio);
        }

        case EnvelopeShape::DoubleTap:
        {
            const float decayRate = -std::log(ENVELOPE_THRESHOLD) / (T * 0.5f);
            const float tap1 = std::exp(-t * decayRate);
            float tap2 = 0.0f;
            const float spacing = T * DOUBLE_TAP_SPACING;
            if (t >= spacing)
                tap2 = std::exp(-(t - spacing) * decayRate);
            return std::max(tap1, tap2);
        }

        case EnvelopeShape::Percussive:
        {
            const float attackSamples = T * PERCUSSIVE_ATTACK_RATIO;
            const float bodySamples = T * PERCUSSIVE_BODY_RATIO;

            if (t < attackSamples)
                return (attackSamples > 0.0f) ? (t / attackSamples) : 1.0f;

            const float tAfterAttack = t - attackSamples;
            if (tAfterAttack < bodySamples)
                return (bodySamples > 0.0f) ? (1.0f - PERCUSSIVE_BODY_DROP * (tAfterAttack / bodySamples)) : (1.0f - PERCUSSIVE_BODY_DROP);

            const float tAfterBody = tAfterAttack - bodySamples;
            const float decayLen = T - attackSamples - bodySamples;
            const float decayRate = (decayLen > 0.0f)
                ? -std::log(ENVELOPE_THRESHOLD / (1.0f - PERCUSSIVE_BODY_DROP)) / decayLen
                : 0.0f;
            return (1.0f - PERCUSSIVE_BODY_DROP) * std::exp(-tAfterBody * decayRate);
        }

        default:
            return 0.0f;
    }
}
