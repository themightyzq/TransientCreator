#include "EnvelopeVisualizer.h"
#include "../LookAndFeel/TransientLookAndFeel.h"

EnvelopeVisualizer::EnvelopeVisualizer(juce::AudioProcessorValueTreeState& apvts,
                                         SharedUIState& sharedState)
    : apvtsRef(apvts), sharedStateRef(sharedState)
{
    shapeParam      = apvts.getRawParameterValue(ParamIDs::SHAPE);
    tailLengthParam = apvts.getRawParameterValue(ParamIDs::TAIL_LENGTH);
    silenceGapParam = apvts.getRawParameterValue(ParamIDs::SILENCE_GAP);
    attackTimeParam = apvts.getRawParameterValue(ParamIDs::ATTACK_TIME);
    holdParam       = apvts.getRawParameterValue(ParamIDs::SUSTAIN_HOLD);

    std::copy(sharedState.customCurveStaging.begin(),
              sharedState.customCurveStaging.end(),
              localCurve.begin());

    apvts.addParameterListener(ParamIDs::SHAPE, this);
    startTimerHz(REPAINT_HZ);

    // Accessibility floor (style guide section 8): a custom component (not a stock JUCE
    // control) needs its own setAccessible/title/description -- a screen reader has no useful
    // name for a bare Component otherwise.
    setTitle("Envelope shape editor");
    setDescription("Interactive envelope curve display: the decay portion is editable -- click "
                    "empty space to add a breakpoint, drag a point to move it, right-click a "
                    "point to delete it, Alt-drag between points to bend the curve's tension. "
                    "Double-click to reset to the selected shape preset.");
    setAccessible(true);

    // Sync breakpoints to the current shape on initial construction
    const auto initialShape = static_cast<EnvelopeShape>(static_cast<int>(shapeParam->load()));
    loadShapeIntoBreakpoints(initialShape);
}

EnvelopeVisualizer::~EnvelopeVisualizer()
{
    apvtsRef.removeParameterListener(ParamIDs::SHAPE, this);
    stopTimer();
}

void EnvelopeVisualizer::timerCallback()
{
    if (sharedStateRef.customCurveLoaded.load(std::memory_order_acquire))
    {
        std::copy(sharedStateRef.customCurveDisplay.begin(),
                  sharedStateRef.customCurveDisplay.end(),
                  localCurve.begin());
        curveIsModified = false;
        sharedStateRef.customCurveLoaded.store(false, std::memory_order_release);
    }
    repaint();
}

void EnvelopeVisualizer::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (parameterID == ParamIDs::SHAPE)
    {
        const auto shape = static_cast<EnvelopeShape>(static_cast<int>(newValue));
        juce::MessageManager::callAsync([this, shape]() {
            loadShapeIntoBreakpoints(shape);
        });
    }
}

void EnvelopeVisualizer::updateLayoutCache()
{
    cachedBounds = getLocalBounds().toFloat().reduced(4.0f);
    const float tailMs = tailLengthParam->load();
    const float gapMs  = silenceGapParam->load();
    const float totalMs = tailMs + gapMs;
    cachedTailFraction = (totalMs > 0.0f) ? (tailMs / totalMs) : TAIL_FRACTION;
    const float attackMs = attackTimeParam->load();
    const float holdPercent = holdParam->load();
    const float attackFrac = (tailMs > 0.0f) ? (attackMs / tailMs) : 0.0f;
    const float holdFrac = holdPercent / 100.0f;
    cachedDecayStart = attackFrac + holdFrac;
}

EnvelopeVisualizer::DecayCoord EnvelopeVisualizer::pixelToDecayCoord(juce::Point<float> pixel) const
{
    DecayCoord result { 0.0f, 0.0f, false };
    const float relX = (pixel.x - cachedBounds.getX()) / cachedBounds.getWidth();
    const float relY = 1.0f - (pixel.y - cachedBounds.getY()) / cachedBounds.getHeight();
    if (relX < 0.0f || relX > cachedTailFraction) return result;
    const float tailNorm = relX / cachedTailFraction;
    if (tailNorm < cachedDecayStart) return result;
    const float decayFrac = 1.0f - cachedDecayStart;
    if (decayFrac <= 0.0f) return result;
    result.normX = juce::jlimit(0.0f, 1.0f, (tailNorm - cachedDecayStart) / decayFrac);
    result.normY = juce::jlimit(0.0f, 1.0f, relY);
    result.inDecayRegion = true;
    return result;
}

juce::Point<float> EnvelopeVisualizer::decayCoordToPixel(float decayNormX, float decayNormY) const
{
    const float decayFrac = 1.0f - cachedDecayStart;
    const float tailNorm = cachedDecayStart + decayNormX * decayFrac;
    const float relX = tailNorm * cachedTailFraction;
    const float px = cachedBounds.getX() + relX * cachedBounds.getWidth();
    const float py = cachedBounds.getBottom() - decayNormY * (cachedBounds.getHeight() - 8.0f) - 4.0f;
    return { px, py };
}

int EnvelopeVisualizer::findNearestBreakpoint(juce::Point<float> pixel, float maxDistPx) const
{
    const auto& bps = sharedStateRef.breakpoints;
    int bestIdx = -1;
    float bestDist = maxDistPx * maxDistPx;
    for (int i = 0; i < static_cast<int>(bps.size()); ++i)
    {
        const auto ptPx = decayCoordToPixel(bps[static_cast<size_t>(i)].x, bps[static_cast<size_t>(i)].y);
        const float dx = pixel.x - ptPx.x;
        const float dy = pixel.y - ptPx.y;
        const float distSq = dx * dx + dy * dy;
        if (distSq < bestDist) { bestDist = distSq; bestIdx = i; }
    }
    return bestIdx;
}

void EnvelopeVisualizer::drawScreenText(juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area,
                                        float px, juce::Justification just, juce::Colour colourOverride) const
{
    if (auto* houseLnF = dynamic_cast<zqsfx::ui::LookAndFeel*>(&getLookAndFeel()))
    {
        houseLnF->drawLcdText(g, text, area.toNearestInt(), px, just, colourOverride);
        return;
    }
    g.setColour(colourOverride);
    g.setFont(juce::FontOptions(px));
    g.drawText(text, area, just);
}

void EnvelopeVisualizer::paint(juce::Graphics& g)
{
    updateLayoutCache();
    const auto& bounds = cachedBounds;
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    namespace colour = zqsfx::ui::colour;

    // Phosphor screen treatment (style guide section 6 / migration spec: custom displays get
    // the house screen background), hard-edged -- no rounded corners.
    zqsfx::ui::LookAndFeel::drawScreen(g, bounds, false);

    const auto shape = static_cast<EnvelopeShape>(static_cast<int>(shapeParam->load()));
    const float tailMs = tailLengthParam->load();
    const float gapMs  = silenceGapParam->load();
    const float attackMs = attackTimeParam->load();
    const float holdPercent = holdParam->load();
    const float attackFraction = (tailMs > 0.0f) ? (attackMs / tailMs) : 0.0f;
    const float holdFraction = holdPercent / 100.0f;
    const float totalMs = tailMs + gapMs;
    const float tailWidth = w * cachedTailFraction;
    const float gapWidth  = w - tailWidth;

    // Gap region -- darker screen well, distinct from the tail area by fill AND by the vertical
    // separator + duration labels below (never colour alone).
    if (gapWidth > 0.0f)
    {
        g.setColour(colour::lcdScreenDark);
        g.fillRect(bounds.getX() + tailWidth, bounds.getY(), gapWidth, h);
    }

    // Attack / hold / decay regions, each in the TIMING or SHAPE channel colour with a DISTINCT
    // fill style so the three read apart without relying on colour alone (style guide section 3
    // rule 2 / migration spec: "attack/sustain regions in the section channel colours with
    // distinct fills -- solid vs hatched or outline").
    if (cachedTailFraction > 0.0f)
    {
        const float attackPixelEnd = bounds.getX() + juce::jlimit(0.0f, 1.0f, attackFraction) * cachedTailFraction * w;
        const float holdPixelEnd   = bounds.getX() + juce::jlimit(0.0f, 1.0f, cachedDecayStart) * cachedTailFraction * w;
        const float decayPixelEnd  = bounds.getX() + tailWidth;

        // ATK: solid low-alpha TIMING fill.
        if (attackPixelEnd > bounds.getX())
        {
            g.setColour(juce::Colour(TransientLookAndFeel::COLOR_TIMING).withAlpha(0.10f));
            g.fillRect(bounds.getX(), bounds.getY(), attackPixelEnd - bounds.getX(), h);
        }

        // HOLD: same TIMING hue, but a diagonal hatch instead of a solid fill -- distinguishes it
        // from ATK's solid fill without changing colour.
        if (holdPixelEnd > attackPixelEnd)
        {
            juce::Graphics::ScopedSaveState save(g);
            g.reduceClipRegion(juce::Rectangle<float>(attackPixelEnd, bounds.getY(),
                                                        holdPixelEnd - attackPixelEnd, h).toNearestInt());
            g.setColour(juce::Colour(TransientLookAndFeel::COLOR_TIMING).withAlpha(0.35f));
            const float step = 6.0f;
            for (float lx = attackPixelEnd - h; lx < holdPixelEnd + h; lx += step)
                g.drawLine(lx, bounds.getBottom(), lx + h, bounds.getY(), 1.0f);
        }

        // DECAY (the editable region): solid low-alpha SHAPE fill -- already a distinct hue from
        // ATK/HOLD's TIMING tint, so this stays a plain fill.
        if (decayPixelEnd > holdPixelEnd)
        {
            g.setColour(juce::Colour(TransientLookAndFeel::COLOR_SHAPE).withAlpha(0.10f));
            g.fillRect(holdPixelEnd, bounds.getY(), decayPixelEnd - holdPixelEnd, h);
        }

        // Small ASCII captions naming each region -- the non-colour cue that makes the fill
        // styles legible as "ATK"/"HOLD"/"DECAY" rather than just three different textures.
        // Placed just below the shape-name/rate row (top), clear of the duration labels and the
        // hover-only instruction hint (both bottom-anchored).
        const float capY = bounds.getY() + 17.0f;
        if (attackPixelEnd - bounds.getX() > 20.0f)
            drawScreenText(g, "ATK", { bounds.getX() + 2.0f, capY, attackPixelEnd - bounds.getX() - 2.0f, 10.0f },
                           8.0f, juce::Justification::centredLeft, colour::lcdFaint2);
        if (holdPixelEnd - attackPixelEnd > 20.0f)
            drawScreenText(g, "HOLD", { attackPixelEnd + 2.0f, capY, holdPixelEnd - attackPixelEnd - 2.0f, 10.0f },
                           8.0f, juce::Justification::centred, colour::lcdFaint2);
        if (decayPixelEnd - holdPixelEnd > 24.0f)
            drawScreenText(g, "DECAY", { holdPixelEnd + 2.0f, capY, decayPixelEnd - holdPixelEnd - 2.0f, 10.0f },
                           8.0f, juce::Justification::centredLeft, colour::lcdFaint2);
    }

    // Separator between tail and gap.
    g.setColour(colour::ruleTitle);
    g.drawVerticalLine(static_cast<int>(bounds.getX() + tailWidth), bounds.getY(), bounds.getBottom());

    // Shape name + edited indicator
    juce::String shapeName = shapeChoices[static_cast<int>(shape)];
    if (curveIsModified) shapeName += " (edited)";
    drawScreenText(g, shapeName, { bounds.getX() + 6.0f, bounds.getY() + 3.0f, 150.0f, 13.0f },
                   12.0f, juce::Justification::centredLeft, colour::lcdDim);

    // Rate display
    if (totalMs > 0.0f)
    {
        const float rateHz = 1000.0f / totalMs;
        juce::String rateStr = juce::String(totalMs, 0) + " ms";
        if (rateHz >= 0.1f && rateHz < 100.0f)
            rateStr += "  " + juce::String(rateHz, 1) + " Hz";
        drawScreenText(g, rateStr, { bounds.getRight() - 160.0f, bounds.getY() + 3.0f, 154.0f, 13.0f },
                       12.0f, juce::Justification::centredRight, colour::lcdFaint);
    }

    // Build envelope path from LUT
    juce::Path envelopePath;
    bool pathStarted = false;
    for (int i = 0; i <= RESOLUTION; ++i)
    {
        const float normalizedX = static_cast<float>(i) / static_cast<float>(RESOLUTION);
        float amplitude = 0.0f;
        if (normalizedX <= cachedTailFraction && cachedTailFraction > 0.0f)
        {
            const float tailNorm = normalizedX / cachedTailFraction;
            amplitude = EnvelopeGenerator::computeShapeAtNormalized(
                tailNorm, shape, attackFraction, holdFraction,
                localCurve.data(), SharedUIState::CUSTOM_CURVE_SIZE);
        }
        const float px = bounds.getX() + normalizedX * w;
        const float py = bounds.getBottom() - amplitude * (h - 8.0f) - 4.0f;
        if (!pathStarted) { envelopePath.startNewSubPath(px, py); pathStarted = true; }
        else envelopePath.lineTo(px, py);
    }

    // The traced envelope curve itself: house lcdText phosphor-green (migration spec: "its
    // envelope curve in lcdText"). Breakpoints/hover/tension/playhead below stay in the SHAPE
    // channel colour -- they are the user-editable AFFORDANCES on top of the curve, not the
    // trace, so a second, deliberately distinct hue marks them as interactive.
    g.setColour(colour::lcdText);
    g.strokePath(envelopePath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path fillPath(envelopePath);
    fillPath.lineTo(bounds.getRight(), bounds.getBottom());
    fillPath.lineTo(bounds.getX(), bounds.getBottom());
    fillPath.closeSubPath();
    g.setColour(colour::lcdText.withAlpha(0.15f));
    g.fillPath(fillPath);

    // Playhead
    const float playheadPos = sharedStateRef.playheadPosition.load(std::memory_order_relaxed);
    const bool inTail = sharedStateRef.isInTail.load(std::memory_order_relaxed);
    if (playheadPos > 0.0f && playheadPos < 1.0f)
    {
        const float px = bounds.getX() + playheadPos * w;
        g.setColour(inTail ? juce::Colour(TransientLookAndFeel::COLOR_SHAPE).withAlpha(0.6f)
                           : juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.3f));
        g.drawVerticalLine(static_cast<int>(px), bounds.getY(), bounds.getBottom());
    }

    // Draw breakpoints
    const auto& bps = sharedStateRef.breakpoints;
    for (int i = 0; i < static_cast<int>(bps.size()); ++i)
    {
        const auto ptPx = decayCoordToPixel(bps[static_cast<size_t>(i)].x, bps[static_cast<size_t>(i)].y);
        const bool isHovered = (i == hoveredIndex);
        const bool isDragged = (i == dragIndex);
        const float r = (isHovered || isDragged) ? POINT_RADIUS + 2.0f : POINT_RADIUS;
        if (isHovered || isDragged)
        {
            g.setColour(juce::Colour(TransientLookAndFeel::COLOR_SHAPE).withAlpha(0.3f));
            g.drawEllipse(ptPx.x - r - 1.0f, ptPx.y - r - 1.0f, (r + 1.0f) * 2.0f, (r + 1.0f) * 2.0f, 1.5f);
        }
        g.setColour(isDragged ? juce::Colour(TransientLookAndFeel::ACCENT_BRIGHT)
                              : juce::Colour(TransientLookAndFeel::COLOR_SHAPE));
        g.fillEllipse(ptPx.x - r, ptPx.y - r, r * 2.0f, r * 2.0f);
    }

    // Tension indicators — only when mouse is hovering in the editor
    if (mouseInside)
    {
        for (int i = 0; i < static_cast<int>(bps.size()) - 1; ++i)
        {
            const float tension = bps[static_cast<size_t>(i)].tension;
            if (std::fabs(tension) < 0.05f) continue;
            const float midX = (bps[static_cast<size_t>(i)].x + bps[static_cast<size_t>(i + 1)].x) * 0.5f;
            const float midY = (bps[static_cast<size_t>(i)].y + bps[static_cast<size_t>(i + 1)].y) * 0.5f;
            const auto midPx = decayCoordToPixel(midX, midY);
            juce::String tensionStr = (tension > 0.0f ? "+" : "") + juce::String(tension, 1);
            drawScreenText(g, tensionStr, { midPx.x - 14.0f, midPx.y - 16.0f, 28.0f, 12.0f },
                           10.0f, juce::Justification::centred, colour::lcdFaint.withAlpha(0.8f));
        }
    }

    // Hover feedback
    if (mouseInside && dragIndex < 0 && tensionDragSegment < 0 && hoveredIndex < 0)
    {
        const auto coord = pixelToDecayCoord(hoverPixel);
        if (coord.inDecayRegion)
        {
            const auto ghostPx = decayCoordToPixel(coord.normX, coord.normY);
            g.setColour(juce::Colour(TransientLookAndFeel::COLOR_SHAPE).withAlpha(0.35f));
            g.fillEllipse(ghostPx.x - POINT_RADIUS, ghostPx.y - POINT_RADIUS, POINT_RADIUS * 2.0f, POINT_RADIUS * 2.0f);
            g.setColour(juce::Colour(TransientLookAndFeel::TEXT_DIM).withAlpha(0.15f));
            g.drawVerticalLine(static_cast<int>(hoverPixel.x), bounds.getY(), bounds.getBottom());
            g.drawHorizontalLine(static_cast<int>(hoverPixel.y), bounds.getX(), bounds.getX() + tailWidth);
            drawScreenText(g, juce::String(coord.normY, 2), { hoverPixel.x + 10.0f, hoverPixel.y - 14.0f, 36.0f, 12.0f },
                           10.0f, juce::Justification::centredLeft, colour::lcdText.withAlpha(0.8f));
        }
    }

    if (mouseInside && hoveredIndex >= 0 && dragIndex < 0)
    {
        const auto& bp = bps[static_cast<size_t>(hoveredIndex)];
        const auto ptPx = decayCoordToPixel(bp.x, bp.y);
        drawScreenText(g, juce::String(bp.y, 2), { ptPx.x + 10.0f, ptPx.y - 14.0f, 36.0f, 12.0f },
                       10.0f, juce::Justification::centredLeft, colour::lcdText.withAlpha(0.9f));
    }

    // Instruction hint (ASCII-only; "|" and "+" are plain ASCII already)
    if (mouseInside && dragIndex < 0 && tensionDragSegment < 0)
    {
        drawScreenText(g, "Click: add point  |  Right-click: delete  |  Alt+drag: bend curve",
                       { bounds.getX(), bounds.getBottom() - 28.0f, tailWidth, 12.0f },
                       9.0f, juce::Justification::centred, colour::lcdFaint2.withAlpha(0.8f));
    }

    // Duration labels
    if (tailWidth > 40.0f)
        drawScreenText(g, juce::String(tailMs, 1) + " ms",
                       { bounds.getX(), bounds.getBottom() - 16.0f, tailWidth, 14.0f },
                       11.0f, juce::Justification::centred, colour::lcdDim);
    if (gapWidth > 40.0f)
        drawScreenText(g, juce::String(gapMs, 1) + " ms",
                       { bounds.getX() + tailWidth, bounds.getBottom() - 16.0f, gapWidth, 14.0f },
                       11.0f, juce::Justification::centred, colour::lcdDim);
}

void EnvelopeVisualizer::resized() { updateLayoutCache(); }

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------

void EnvelopeVisualizer::mouseDown(const juce::MouseEvent& event)
{
    updateLayoutCache();

    // Right-click = delete point
    if (event.mods.isRightButtonDown())
    {
        const int idx = findNearestBreakpoint(event.position, POINT_HIT_RADIUS);
        if (idx >= 0)
        {
            auto& bps = sharedStateRef.breakpoints;
            if (idx > 0 && idx < static_cast<int>(bps.size()) - 1)
            {
                bps.erase(bps.begin() + idx);
                hoveredIndex = -1;
                curveIsModified = true;
                rebuildAndPush();
            }
        }
        return;
    }

    // Alt+click = start tension drag
    if (event.mods.isAltDown())
    {
        const auto coord = pixelToDecayCoord(event.position);
        if (coord.inDecayRegion)
        {
            const auto& bps = sharedStateRef.breakpoints;
            for (int i = 0; i < static_cast<int>(bps.size()) - 1; ++i)
            {
                if (coord.normX >= bps[static_cast<size_t>(i)].x
                    && coord.normX < bps[static_cast<size_t>(i + 1)].x)
                {
                    tensionDragSegment = i;
                    tensionDragStartY = event.position.y;
                    tensionDragStartVal = bps[static_cast<size_t>(i)].tension;
                    break;
                }
            }
        }
        return;
    }

    // Try to grab existing point
    const int idx = findNearestBreakpoint(event.position, POINT_HIT_RADIUS);
    if (idx >= 0) { dragIndex = idx; return; }

    // Add new point
    const auto coord = pixelToDecayCoord(event.position);
    if (coord.inDecayRegion)
    {
        auto& bps = sharedStateRef.breakpoints;
        bps.push_back({ coord.normX, coord.normY, 0.0f });
        std::sort(bps.begin(), bps.end());
        for (int i = 0; i < static_cast<int>(bps.size()); ++i)
        {
            if (std::fabs(bps[static_cast<size_t>(i)].x - coord.normX) < 1.0e-5f
                && std::fabs(bps[static_cast<size_t>(i)].y - coord.normY) < 1.0e-5f)
            { dragIndex = i; break; }
        }
        curveIsModified = true;
        rebuildAndPush();
    }
}

void EnvelopeVisualizer::mouseDrag(const juce::MouseEvent& event)
{
    // Tension drag
    if (tensionDragSegment >= 0)
    {
        auto& bps = sharedStateRef.breakpoints;
        if (tensionDragSegment < static_cast<int>(bps.size()))
        {
            const float deltaY = tensionDragStartY - event.position.y;
            const float tensionDelta = deltaY / 100.0f;
            bps[static_cast<size_t>(tensionDragSegment)].tension =
                juce::jlimit(-1.0f, 1.0f, tensionDragStartVal + tensionDelta);
            curveIsModified = true;
            rebuildAndPush();
        }
        return;
    }

    if (dragIndex < 0) return;
    updateLayoutCache();
    auto& bps = sharedStateRef.breakpoints;
    if (dragIndex >= static_cast<int>(bps.size())) return;

    const auto coord = pixelToDecayCoord(event.position);
    auto& bp = bps[static_cast<size_t>(dragIndex)];

    if (dragIndex == 0)
    {
        bp.x = 0.0f;
        bp.y = juce::jlimit(0.0f, 1.0f, coord.inDecayRegion ? coord.normY
               : 1.0f - (event.position.y - cachedBounds.getY()) / cachedBounds.getHeight());
    }
    else if (dragIndex == static_cast<int>(bps.size()) - 1)
    {
        bp.x = 1.0f;
        bp.y = juce::jlimit(0.0f, 1.0f, coord.inDecayRegion ? coord.normY
               : 1.0f - (event.position.y - cachedBounds.getY()) / cachedBounds.getHeight());
    }
    else
    {
        const float minX = bps[static_cast<size_t>(dragIndex - 1)].x + 0.001f;
        const float maxX = bps[static_cast<size_t>(dragIndex + 1)].x - 0.001f;
        bp.x = juce::jlimit(minX, maxX, coord.inDecayRegion ? coord.normX : bp.x);
        bp.y = juce::jlimit(0.0f, 1.0f, coord.inDecayRegion ? coord.normY
               : 1.0f - (event.position.y - cachedBounds.getY()) / cachedBounds.getHeight());
    }

    curveIsModified = true;
    rebuildAndPush();
}

void EnvelopeVisualizer::mouseUp(const juce::MouseEvent&)
{
    dragIndex = -1;
    tensionDragSegment = -1;
}

void EnvelopeVisualizer::mouseMove(const juce::MouseEvent& event)
{
    mouseInside = true;
    hoverPixel = event.position;
    updateLayoutCache();
    hoveredIndex = findNearestBreakpoint(event.position, POINT_HIT_RADIUS);

    if (event.mods.isAltDown() && hoveredIndex < 0)
    {
        const auto coord = pixelToDecayCoord(event.position);
        if (coord.inDecayRegion)
        { setMouseCursor(juce::MouseCursor::UpDownResizeCursor); return; }
    }

    if (hoveredIndex >= 0)
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    else
    {
        const auto coord = pixelToDecayCoord(event.position);
        setMouseCursor(coord.inDecayRegion ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor);
    }
}

void EnvelopeVisualizer::mouseDoubleClick(const juce::MouseEvent&)
{
    const auto shape = static_cast<EnvelopeShape>(static_cast<int>(shapeParam->load()));
    loadShapeIntoBreakpoints(shape);
}

void EnvelopeVisualizer::mouseExit(const juce::MouseEvent&)
{
    mouseInside = false;
    hoveredIndex = -1;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

// ---------------------------------------------------------------------------
// Breakpoint / LUT helpers
// ---------------------------------------------------------------------------

void EnvelopeVisualizer::rebuildAndPush()
{
    sharedStateRef.rebuildLUTFromBreakpoints();
    std::copy(sharedStateRef.customCurveStaging.begin(),
              sharedStateRef.customCurveStaging.end(), localCurve.begin());
    sharedStateRef.customCurveUpdated.store(true, std::memory_order_release);
}

void EnvelopeVisualizer::loadShapeIntoBreakpoints(EnvelopeShape shape)
{
    auto& bps = sharedStateRef.breakpoints;
    bps.clear();

    switch (shape)
    {
        case EnvelopeShape::Exponential:
        {
            bps.push_back({ 0.00f, 1.00f,  0.7f });
            bps.push_back({ 1.00f, 0.03f,  0.0f });
            break;
        }
        case EnvelopeShape::Linear:
        {
            bps.push_back({ 0.00f, 1.00f,  0.0f });
            bps.push_back({ 1.00f, 0.01f,  0.0f });
            break;
        }
        case EnvelopeShape::Logarithmic:
        {
            bps.push_back({ 0.00f, 1.00f, -0.7f });
            bps.push_back({ 1.00f, 0.02f,  0.0f });
            break;
        }
        case EnvelopeShape::ReverseSawtooth:
        {
            bps.push_back({ 0.00f, 1.00f,  0.0f });
            bps.push_back({ 0.15f, 1.00f,  0.0f });
            bps.push_back({ 1.00f, 0.01f,  0.0f });
            break;
        }
        case EnvelopeShape::DoubleTap:
        {
            bps.push_back({ 0.00f, 1.00f,  0.3f });
            bps.push_back({ 0.12f, 0.20f,  0.0f });
            bps.push_back({ 0.20f, 0.12f, -0.2f });
            bps.push_back({ 0.30f, 0.75f,  0.3f });
            bps.push_back({ 0.45f, 0.15f,  0.2f });
            bps.push_back({ 1.00f, 0.02f,  0.0f });
            break;
        }
        case EnvelopeShape::Percussive:
        {
            bps.push_back({ 0.00f, 1.00f,  0.0f });
            bps.push_back({ 0.15f, 0.70f,  0.5f });
            bps.push_back({ 1.00f, 0.03f,  0.0f });
            break;
        }
        default:
        {
            bps.push_back({ 0.00f, 1.00f, 0.5f });
            bps.push_back({ 1.00f, 0.03f, 0.0f });
            break;
        }
    }

    curveIsModified = false;
    rebuildAndPush();
}
