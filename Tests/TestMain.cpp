// TestMain.cpp — console entry point for the JUCE UnitTest-based DSP test suite.
// Returns a non-zero exit code if any check fails, so CI/CTest report failures.

#include <juce_core/juce_core.h>
#include <iostream>

namespace
{
    // Routes JUCE UnitTest output to stdout (the default logger is silent in a
    // release console build).
    class ConsoleTestRunner : public juce::UnitTestRunner
    {
        void logMessage (const juce::String& message) override
        {
            std::cout << message << std::endl;
        }
    };
}

int main()
{
    ConsoleTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.runAllTests();

    int totalChecks = 0;
    int totalFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (const auto* r = runner.getResult (i))
        {
            totalChecks   += r->passes + r->failures;
            totalFailures += r->failures;
        }
    }

    std::cout << "\n==== " << totalChecks << " checks, "
              << totalFailures << " failure(s) ====" << std::endl;

    return totalFailures > 0 ? 1 : 0;
}
