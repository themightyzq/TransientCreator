// transient_editor_lifetime_tests: drives the real processor and editor (linked against the
// plugin's shared-code target, the same pattern as transient_preset_tests) through the
// sequence behind the intermittent pluginval crash in v1.1.2:
//
//   the host automates the Shape parameter while the editor is open (from the audio thread
//   and from the message thread), then closes the editor before the message loop runs again.
//
// Up to v1.1.2, EnvelopeVisualizer::parameterChanged() posted MessageManager::callAsync with a
// lambda capturing the raw component pointer. A lambda still queued when the editor was deleted
// ran on freed memory: it rewrote the processor's curve breakpoints and audio LUT through a
// dangling reference, or crashed (pluginval "Segmentation fault: 11"). In a plugin hosted
// in-process the host's message queue is separate from the plugin's, so "delete the editor"
// can overtake the queued lambda.
//
// Checks:
//   1. After the editor is gone, pending Shape notifications must not touch the processor's
//      curve state (old code: the state is overwritten, or the process crashes).
//   2. With the editor open, a Shape change still loads that shape's preset curve.
//
// macOS only: the shared-code target is built without JUCE_MODAL_LOOPS_PERMITTED, so the
// message loop is pumped with CFRunLoopRunInMode. Elsewhere the program reports a skip.
// Prints one line per check and returns non-zero on any failure or if a check did not run.

#if defined(__APPLE__)
 #include <CoreFoundation/CoreFoundation.h>
#endif

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
#include <thread>

#if JUCE_MAC
namespace
{
    int failureCount = 0;
    int checksRun = 0;
    constexpr int EXPECTED_CHECKS = 6;

    // Long enough for several EnvelopeVisualizer timer ticks (30 Hz) and for every queued
    // message to be delivered.
    constexpr int SETTLE_MS = 300;
    constexpr int NOTIFY_ROUNDS = 50;

    void check(bool condition, const juce::String& what)
    {
        ++checksRun;
        std::cout << (condition ? "pass: " : "FAIL: ") << what << std::endl;
        if (!condition)
            ++failureCount;
    }

    void pumpMessages(int milliseconds)
    {
        const auto end = juce::Time::getMillisecondCounter() + static_cast<juce::uint32>(milliseconds);
        while (juce::Time::getMillisecondCounter() < end)
            CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, true);
    }

    float normalisedShape(EnvelopeShape shape)
    {
        const auto numChoices = static_cast<float>(shapeChoices.size());
        return static_cast<float>(static_cast<int>(shape)) / (numChoices - 1.0f);
    }

    // A curve no shape preset produces, so any preset load after the editor closed shows up.
    void installSentinelCurve(SharedUIState& state)
    {
        state.breakpoints = { { 0.0f, 0.9f, 0.0f }, { 0.33f, 0.41f, 0.0f },
                              { 0.66f, 0.27f, 0.0f }, { 1.0f, 0.05f, 0.0f } };
        state.rebuildLUTFromBreakpoints();
        state.customCurveUpdated.store(false);
    }

    bool sameBreakpoints(const std::vector<SharedUIState::Breakpoint>& a,
                         const std::vector<SharedUIState::Breakpoint>& b)
    {
        if (a.size() != b.size())
            return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (!juce::exactlyEqual(a[i].x, b[i].x) || !juce::exactlyEqual(a[i].y, b[i].y)
                || !juce::exactlyEqual(a[i].tension, b[i].tension))
                return false;
        return true;
    }

    // --- 1. Notifications pending at editor close must not outlive the editor ---------------
    void checkNotificationsDoNotOutliveEditor(TransientCreatorProcessor& processor)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto* shape = processor.apvts.getParameter(ParamIDs::SHAPE);

        // Automation from a non-message thread (the audio thread in a host) ...
        std::thread automation([shape]
        {
            for (int i = 0; i < NOTIFY_ROUNDS; ++i)
                shape->setValueNotifyingHost(normalisedShape(i % 2 == 0 ? EnvelopeShape::DoubleTap
                                                                        : EnvelopeShape::Percussive));
        });
        automation.join();

        // ... and from the message thread (a VST3 host's edit-controller path).
        for (int i = 0; i < NOTIFY_ROUNDS; ++i)
            shape->setValueNotifyingHost(normalisedShape(i % 2 == 0 ? EnvelopeShape::Linear
                                                                    : EnvelopeShape::DoubleTap));

        // The host closes the editor before the message loop runs again.
        editor.reset();

        auto& state = processor.sharedState;
        installSentinelCurve(state);
        const auto sentinelBreakpoints = state.breakpoints;
        const auto sentinelLUT = state.customCurveStaging;

        pumpMessages(SETTLE_MS);

        check(sameBreakpoints(state.breakpoints, sentinelBreakpoints),
              "breakpoints untouched by Shape notifications pending when the editor closed");
        check(state.customCurveStaging == sentinelLUT,
              "audio curve LUT untouched by Shape notifications pending when the editor closed");
        check(!state.customCurveUpdated.load(),
              "no curve update pushed to the audio thread after the editor closed");
    }

    // --- 2. Shape change with the editor open still loads the preset ------------------------
    void checkShapeChangeWhileOpenApplies(TransientCreatorProcessor& processor)
    {
        auto* shape = processor.apvts.getParameter(ParamIDs::SHAPE);
        shape->setValueNotifyingHost(normalisedShape(EnvelopeShape::Linear));

        // The editor syncs the curve to the current shape (Linear, two points) on construction.
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        check(processor.sharedState.breakpoints.size() == 2,
              "a new editor loads the current shape's (Linear) preset curve");

        shape->setValueNotifyingHost(normalisedShape(EnvelopeShape::DoubleTap));
        pumpMessages(SETTLE_MS);

        // DoubleTap is the only preset with six breakpoints.
        check(processor.sharedState.breakpoints.size() == 6,
              "with the editor open, a Shape change loads that shape's preset curve");
        check(processor.sharedState.customCurveUpdated.load(),
              "with the editor open, the new curve is pushed to the audio thread");
    }
}
#endif

int main()
{
   #if JUCE_MAC
    juce::ScopedJuceInitialiser_GUI gui;
    {
        TransientCreatorProcessor processor;
        checkNotificationsDoNotOutliveEditor(processor);
        checkShapeChangeWhileOpenApplies(processor);
    }

    if (checksRun != EXPECTED_CHECKS)
    {
        std::cout << "FAIL: ran " << checksRun << " of " << EXPECTED_CHECKS << " checks" << std::endl;
        ++failureCount;
    }

    std::cout << "\n==== " << failureCount << " failure(s) ====" << std::endl;
    return failureCount > 0 ? 1 : 0;
   #else
    std::cout << "skip: editor lifetime checks pump the macOS run loop; macOS CI runs them" << std::endl;
    return 0;
   #endif
}
