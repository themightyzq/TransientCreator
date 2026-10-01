// transient_preset_tests: constructs the real TransientCreatorProcessor (linked against the
// plugin's own shared-code target, the same "pamplejuce pattern" tools/ui_snapshot uses -- see
// CMakeLists.txt) and exercises the preset mechanism end to end:
//
//   1. Every factory preset (via PresetManager) differs from "Init" in at least one parameter.
//   2. A user preset saved to a TEMP directory (never the real
//      ~/Library/Audio/Presets/ZQ SFX/Transient Creator/) round-trips its name through
//      PluginProcessor::getStateInformation() / setStateInformation() -- the mechanism a real
//      host session uses to remember the loaded preset.
//
// Plain main(), no JUCE UnitTest framework (this needs the real plugin/editor JuceHeader.h,
// which TestMain.cpp's isolated DSP-only test binary does not link against). Prints one line
// per check and returns non-zero if any failed, so CTest reports failures.

#include "PluginProcessor.h"
#include "Presets/PresetManager.h"
#include <iostream>
#include <cmath>

namespace
{
    int failureCount = 0;

    void check(bool condition, const juce::String& what)
    {
        if (condition)
        {
            std::cout << "pass: " << what << std::endl;
        }
        else
        {
            std::cout << "FAIL: " << what << std::endl;
            ++failureCount;
        }
    }

    std::vector<float> snapshotNormalisedValues(TransientCreatorProcessor& proc)
    {
        std::vector<float> values;
        for (auto* param : proc.getParameters())
            values.push_back(param->getValue());
        return values;
    }
}

int main()
{
    juce::ScopedJuceInitialiser_GUI gui;

    // Processor declared before its PresetManager reference target's own destruction order
    // does not matter here (PresetManager takes a reference, does not own the processor).
    TransientCreatorProcessor processor;
    juce::String err;

    // --- 1. Factory presets vs Init -----------------------------------------------------
    tc::PresetManager presets(processor);

    check(!presets.getEntries().empty() && presets.getEntries()[0].name == "Init" && presets.getEntries()[0].isInit,
          "entry 0 is the synthetic \"Init\" entry");

    check(presets.load(0, err), "load Init (" + err + ")");
    const auto initValues = snapshotNormalisedValues(processor);

    int factoryCount = 0;
    for (int i = 0; i < static_cast<int>(presets.getEntries().size()); ++i)
    {
        const auto& e = presets.getEntries()[static_cast<size_t>(i)];
        if (e.isInit || e.isUser)
            continue;
        ++factoryCount;

        const bool loaded = presets.load(i, err);
        check(loaded, "load factory preset \"" + e.name + "\" (" + err + ")");
        if (!loaded)
            continue;

        const auto values = snapshotNormalisedValues(processor);
        bool anyDiffers = false;
        for (size_t p = 0; p < values.size() && p < initValues.size(); ++p)
        {
            if (std::abs(values[p] - initValues[p]) > 1.0e-6f)
            {
                anyDiffers = true;
                break;
            }
        }
        check(anyDiffers, "preset \"" + e.name + "\" differs from Init in at least one parameter");
    }
    check(factoryCount > 0, "at least one factory preset was found (BinaryData from Presets/*.tcpreset)");

    // --- 2. User preset: save to a TEMP dir, reload, round-trip the name through -----------
    //        getStateInformation()/setStateInformation() (never touches the real user folder).
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                       .getChildFile("TransientCreatorPresetTest_"
                                     + juce::String(juce::Random::getSystemRandom().nextInt(1000000)));
    tempDir.deleteRecursively();

    {
        tc::PresetManager tempPresets(processor, tempDir);

        // Dial in a recognisable, non-default state before saving.
        check(tempPresets.load(1, err), "load a factory preset before saving a user preset (" + err + ")");

        const juce::String userName = "Round Trip Test";
        check(tempPresets.saveUser(userName, err), "saveUser writes a .tcpreset to the temp dir (" + err + ")");
        check(tempDir.getChildFile(userName + ".tcpreset").existsAsFile(),
              "the saved file exists in the temp dir, not the real user folder");

        // Move the processor away from that preset so the reload below is a real test.
        check(tempPresets.load(0, err), "move back to Init before reloading the user preset");

        tempPresets.rescan();
        const int userIndex = tempPresets.indexOf(userName);
        check(userIndex >= 0, "rescan finds the saved user preset");
        check(userIndex >= 0 && tempPresets.load(userIndex, err), "load the user preset back (" + err + ")");
        check(processor.apvts.state.getProperty("presetName").toString() == userName,
              "presetName property is set after loading the user preset");

        // Full session round trip: serialize, disturb the state, deserialize, check the name.
        juce::MemoryBlock savedState;
        processor.getStateInformation(savedState);

        check(tempPresets.load(0, err), "disturb state (load Init) before restoring");
        check(processor.apvts.state.getProperty("presetName").toString() != userName,
              "presetName no longer matches after disturbing the state");

        processor.setStateInformation(savedState.getData(), static_cast<int>(savedState.getSize()));
        check(processor.apvts.state.getProperty("presetName").toString() == userName,
              "presetName round-trips through getStateInformation()/setStateInformation()");
    }

    tempDir.deleteRecursively();

    std::cout << "\n==== " << failureCount << " failure(s) ====" << std::endl;
    return failureCount > 0 ? 1 : 0;
}
