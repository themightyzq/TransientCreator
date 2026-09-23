#pragma once
// Preset browsing for Transient Creator. Factory presets are compiled into the binary
// (BinaryData, generated from Presets/*.tcpreset -- see CMakeLists.txt) so they travel
// inside the VST3/AU bundle; user presets are the same XML format written to
// ~/Library/Audio/Presets/ZQ SFX/Transient Creator/. Modelled on Broken's PresetManager
// (Project_TurboSynth/plugin/src/plugin/PresetManager.h), adapted for APVTS XML state
// instead of a hand-rolled JSON snapshot.
//
// A .tcpreset file is exactly the XML produced by apvts.copyState() (i.e. what
// getStateInformation() would write, minus the session-only breakpoint/curve/editor-size
// properties), plus a "presetName" attribute on the root element. Loading one therefore
// only ever touches the 16 APVTS parameters -- it never disturbs the hand-drawn envelope
// curve or the editor size.
//
// Message-thread only: every entry point here does file I/O and/or calls
// setValueNotifyingHost(), neither of which is safe on the audio thread.

#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>

class TransientCreatorProcessor;

namespace tc
{

class PresetManager
{
public:
    struct Entry
    {
        juce::String name;      // display name / combo box entry / file stem
        bool isInit = false;    // the synthetic "Init" entry (parameter defaults)
        bool isUser = false;    // true for entries loaded from disk, false for Init/factory
        int binaryIndex = -1;   // factory: index into BinaryData::namedResourceList
        juce::File file;        // user: the file on disk
    };

    // userDirOverride is test-only: production code always uses defaultUserDirectory().
    explicit PresetManager(TransientCreatorProcessor& p, const juce::File& userDirOverride = {});

    static juce::File defaultUserDirectory();
    juce::File getUserDirectory() const;

    void rescan();
    const std::vector<Entry>& getEntries() const { return entries; }
    int getCurrentIndex() const { return currentIndex; }
    juce::String getCurrentName() const;
    int indexOf(const juce::String& name) const;

    bool load(int index, juce::String& errorOut);
    bool step(int delta, juce::String& errorOut);
    bool saveUser(const juce::String& name, juce::String& errorOut);

    // USER-preset management; each refuses Init/factory entries.
    bool renameUser(int index, const juce::String& newName, juce::String& errorOut);
    bool deleteUser(int index, juce::String& errorOut);

    // Resyncs currentIndex (and its displayed name) to the "presetName" property currently
    // on apvts.state -- called after a full session restore (setStateInformation), which
    // replaces apvts.state wholesale without going through load().
    void syncCurrentIndexFromState();

private:
    void loadInit();
    bool loadStateText(const juce::String& xmlText, juce::String& errorOut);

    TransientCreatorProcessor& processor;
    juce::File userDirOverride;
    std::vector<Entry> entries;
    int currentIndex = -1;
};

} // namespace tc
