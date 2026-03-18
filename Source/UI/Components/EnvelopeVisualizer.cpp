#include "EnvelopeVisualizer.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

EnvelopeVisualizer::EnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts)
{
    shapeParam      = apvts.getRawParameterValue(ParamIDs::SHAPE);
    tailLengthParam = apvts.getRawParameterValue(ParamIDs::TAIL_LENGTH);
    silenceGapParam = apvts.getRawParameterValue(ParamIDs::SILENCE_GAP);
    attackTimeParam = apvts.getRawParameterValue(ParamIDs::ATTACK_TIME);
    tensionParam    = apvts.getRawParameterValue(ParamIDs::ENVELOPE_TENSION);
    holdParam       = apvts.getRawParameterValue(ParamIDs::SUSTAIN_HOLD);

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
    g.setColour(juce::Colour(TransientLookAndFeel::BG_DARK));
    g.fillRoundedRectangle(bounds, 4.0f);

    const auto shape      = static_cast<EnvelopeShape>(static_cast<int>(shapeParam->load()));
    const float tailMs    = tailLengthParam->load();
    const float gapMs     = silenceGapParam->load();
    const float attackMs  = attackTimeParam->load();
    const float tension   = tensionParam->load();
    const float holdPercent = holdParam->load();

    const float referenceTailSamples = (tailMs / 1000.0f) * REFERENCE_RATE;
    const float attackFraction = (tailMs > 0.0f) ? (attackMs / tailMs) : 0.0f;
    const float holdFraction = holdPercent / 100.0f;

    const float totalMs = tailMs + gapMs;
    const float tailFraction = (totalMs > 0.0f) ? (tailMs / totalMs) : TAIL_FRACTION;
    const float tailWidth = w * tailFraction;
    const float gapWidth  = w - tailWidth;

    // Gap region
    if (gapWidth > 0.0f)
    {
        g.setColour(juce::Colour(TransientLookAndFeel::BG_PANEL));
        g.fillRoundedRectangle(bounds.getX() + tailWidth, bounds.getY(), gapWidth, h, 0.0f);
    }

    // Separator
    g.setColour(juce::Colour(TransientLookAndFeel::KNOB_TRACK));
    g.drawVerticalLine(static_cast<int>(bounds.getX() + tailWidth), bounds.getY(), bounds.getBottom());

    // Shape name in top-left
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.5f));
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(shapeChoices[static_cast<int>(shape)],
               juce::Rectangle<float>(bounds.getX() + 6.0f, bounds.getY() + 4.0f, 100.0f, 12.0f),
               juce::Justification::centredLeft);

    // Region labels
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.35f));
    g.setFont(juce::FontOptions(9.0f));
    if (tailWidth > 40.0f)
        g.drawText("TAIL", juce::Rectangle<float>(bounds.getX(), bounds.getY() + 4.0f, tailWidth, 10.0f),
                   juce::Justification::centred);
    if (gapWidth > 40.0f)
        g.drawText("GAP", juce::Rectangle<float>(bounds.getX() + tailWidth, bounds.getY() + 4.0f, gapWidth, 10.0f),
                   juce::Justification::centred);

    // Build envelope path
    juce::Path envelopePath;
    bool pathStarted = false;

    for (int i = 0; i <= RESOLUTION; ++i)
    {
        const float normalizedX = static_cast<float>(i) / static_cast<float>(RESOLUTION);
        float amplitude = 0.0f;

        if (normalizedX <= tailFraction && tailFraction > 0.0f)
        {
            const float tailNorm = normalizedX / tailFraction;
            amplitude = EnvelopeGenerator::computeShapeAtNormalized(
                tailNorm, shape, referenceTailSamples, tension, attackFraction, holdFraction);
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

    // Draw curve
    g.setColour(juce::Colour(TransientLookAndFeel::ACCENT));
    g.strokePath(envelopePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));

    // Fill under curve
    juce::Path fillPath(envelopePath);
    fillPath.lineTo(bounds.getRight(), bounds.getBottom());
    fillPath.lineTo(bounds.getX(), bounds.getBottom());
    fillPath.closeSubPath();
    g.setColour(juce::Colour(TransientLookAndFeel::ACCENT).withAlpha(0.15f));
    g.fillPath(fillPath);

    // Duration labels
    g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM));
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
}
