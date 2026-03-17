#include "EnvelopeGenerator.h"

void EnvelopeGenerator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
}

void EnvelopeGenerator::trigger()
{
    active = true;
    // Stub: reset phase counters — implemented in Phase 2
}

float EnvelopeGenerator::getNextSample()
{
    // Stub: returns 1.0 (passthrough) — implemented in Phase 2
    return 1.0f;
}

bool EnvelopeGenerator::isActive() const
{
    return active;
}

void EnvelopeGenerator::setTailLength(float ms)
{
    tailLengthMs = ms;
}

void EnvelopeGenerator::setSilenceGap(float ms)
{
    silenceGapMs = ms;
}

void EnvelopeGenerator::setShape(EnvelopeShape shape)
{
    currentShape = shape;
}
