#include "EnvelopeVisualizer.h"

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

    // Reference tail samples for shape computation (uses fixed reference rate)
    const float referenceTailSamples = (tailMs / 1000.0f) * REFERENCE_RATE;

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

    // Build envelope path using the authoritative static method from EnvelopeGenerator
    juce::Path envelopePath;
    bool pathStarted = false;

    for (int i = 0; i <= RESOLUTION; ++i)
    {
        const float normalizedX = static_cast<float>(i) / static_cast<float>(RESOLUTION);
        float amplitude = 0.0f;

        if (normalizedX <= tailFraction && tailFraction > 0.0f)
        {
            const float tailNorm = normalizedX / tailFraction;
            amplitude = EnvelopeGenerator::computeShapeAtNormalized(tailNorm, shape, referenceTailSamples);
        }

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
