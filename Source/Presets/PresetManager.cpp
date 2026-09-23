#include "PresetManager.h"
#include "../PluginProcessor.h"
#include <BinaryData.h>
#include <algorithm>
#include <cstddef>

namespace tc
{

namespace
{
    constexpr int TCPRESET_EXTENSION_LENGTH = 9; // ".tcpreset"
}

PresetManager::PresetManager(TransientCreatorProcessor& p, const juce::File& userDirOverride_)
    : processor(p), userDirOverride(userDirOverride_)
{
    rescan();

    // A freshly-constructed APVTS already holds every parameter's default value, i.e. Init --
    // just without a "presetName" property yet. Label it as such (a plain ValueTree property
    // set, not a parameter change, so this does not dirty the host's "unsaved changes" state)
    // so the preset bar shows something on first open instead of a blank combo box. A real
    // session restore overwrites this immediately after via syncCurrentIndexFromState().
    currentIndex = indexOf("Init");
    processor.apvts.state.setProperty("presetName", "Init", nullptr);
}

juce::File PresetManager::defaultUserDirectory()
{
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library/Audio/Presets/ZQ SFX/Transient Creator");
}

juce::File PresetManager::getUserDirectory() const
{
    return userDirOverride != juce::File() ? userDirOverride : defaultUserDirectory();
}

void PresetManager::rescan()
{
    entries.clear();

    // "Init" is synthetic (not a file/resource): parameter defaults.
    entries.push_back({ "Init", true, false, -1, {} });

    const size_t firstFactory = entries.size();
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String original(BinaryData::originalFilenames[i]);
        if (!original.endsWithIgnoreCase(".tcpreset"))
            continue;
        entries.push_back({ original.dropLastCharacters(TCPRESET_EXTENSION_LENGTH), false, false, i, {} });
    }
    std::sort(entries.begin() + static_cast<std::ptrdiff_t>(firstFactory), entries.end(),
              [](const Entry& a, const Entry& b) { return a.name < b.name; });

    auto dir = getUserDirectory();
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles(juce::File::findFiles, false, "*.tcpreset");
        files.sort();
        for (const auto& f : files)
            entries.push_back({ f.getFileNameWithoutExtension(), false, true, -1, f });
    }
}

juce::String PresetManager::getCurrentName() const
{
    if (currentIndex >= 0 && currentIndex < static_cast<int>(entries.size()))
        return entries[static_cast<size_t>(currentIndex)].name;
    return {};
}

int PresetManager::indexOf(const juce::String& name) const
{
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].name.equalsIgnoreCase(name))
            return static_cast<int>(i);
    return -1;
}

void PresetManager::loadInit()
{
    // getDefaultValue()/setValueNotifyingHost() are both plain AudioProcessorParameter
    // members, so every automatable parameter can be reset without a downcast.
    for (auto* param : processor.getParameters())
        param->setValueNotifyingHost(param->getDefaultValue());

    processor.apvts.state.setProperty("presetName", "Init", nullptr);
}

bool PresetManager::loadStateText(const juce::String& xmlText, juce::String& errorOut)
{
    auto xml = juce::XmlDocument::parse(xmlText);
    if (xml == nullptr)
    {
        errorOut = "preset file is not valid XML";
        return false;
    }
    if (!xml->hasTagName(processor.apvts.state.getType()))
    {
        errorOut = "preset file has the wrong root element";
        return false;
    }

    auto state = juce::ValueTree::fromXml(*xml);
    processor.apvts.replaceState(state);
    return true;
}

bool PresetManager::load(int index, juce::String& errorOut)
{
    if (index < 0 || index >= static_cast<int>(entries.size()))
    {
        errorOut = "no such preset";
        return false;
    }
    const auto& e = entries[static_cast<size_t>(index)];

    if (e.isInit)
    {
        loadInit();
        currentIndex = index;
        return true;
    }

    juce::String text;
    if (e.isUser)
    {
        text = e.file.loadFileAsString();
    }
    else
    {
        int size = 0;
        if (const char* data = BinaryData::getNamedResource(
                BinaryData::namedResourceList[e.binaryIndex], size))
            text = juce::String::fromUTF8(data, size);
    }

    if (text.isEmpty())
    {
        errorOut = "preset is empty: " + e.name;
        return false;
    }

    if (!loadStateText(text, errorOut))
        return false;

    // Keep the state's presetName in sync with the entry's current display name (it may
    // have been renamed since the file was written).
    processor.apvts.state.setProperty("presetName", e.name, nullptr);
    currentIndex = index;
    return true;
}

bool PresetManager::step(int delta, juce::String& errorOut)
{
    if (entries.empty())
        return false;
    const int n = static_cast<int>(entries.size());
    const int idx = currentIndex < 0 ? 0 : ((currentIndex + delta) % n + n) % n;
    return load(idx, errorOut);
}

bool PresetManager::saveUser(const juce::String& name, juce::String& errorOut)
{
    auto dir = getUserDirectory();
    if (!dir.isDirectory() && !dir.createDirectory())
    {
        errorOut = "cannot create " + dir.getFullPathName();
        return false;
    }

    auto safe = juce::File::createLegalFileName(name.trim());
    if (safe.isEmpty())
    {
        errorOut = "please give the preset a name";
        return false;
    }

    auto f = dir.getChildFile(safe + ".tcpreset");

    auto state = processor.apvts.copyState();
    state.setProperty("presetName", safe, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    if (xml == nullptr || !xml->writeTo(f))
    {
        errorOut = "cannot write " + f.getFullPathName();
        return false;
    }

    processor.apvts.state.setProperty("presetName", safe, nullptr);
    rescan();
    currentIndex = indexOf(safe);
    return true;
}

bool PresetManager::renameUser(int index, const juce::String& newName, juce::String& errorOut)
{
    if (index < 0 || index >= static_cast<int>(entries.size()) || !entries[static_cast<size_t>(index)].isUser)
    {
        errorOut = "only user presets can be renamed";
        return false;
    }
    auto safe = juce::File::createLegalFileName(newName.trim());
    if (safe.isEmpty())
    {
        errorOut = "please give the preset a name";
        return false;
    }

    auto& e = entries[static_cast<size_t>(index)];
    auto target = e.file.getSiblingFile(safe + ".tcpreset");
    if (target == e.file)
        return true;
    if (target.existsAsFile())
    {
        errorOut = "a preset named \"" + safe + "\" already exists";
        return false;
    }
    if (!e.file.moveFileTo(target))
    {
        errorOut = "cannot rename " + e.file.getFileName();
        return false;
    }

    const bool wasCurrent = currentIndex == index;
    rescan();
    if (wasCurrent)
    {
        currentIndex = indexOf(safe);
        processor.apvts.state.setProperty("presetName", safe, nullptr);
    }
    return true;
}

bool PresetManager::deleteUser(int index, juce::String& errorOut)
{
    if (index < 0 || index >= static_cast<int>(entries.size()) || !entries[static_cast<size_t>(index)].isUser)
    {
        errorOut = "only user presets can be deleted";
        return false;
    }
    // moveToTrash, not deleteFile: a preset is a sound design (recoverable).
    if (!entries[static_cast<size_t>(index)].file.moveToTrash())
    {
        errorOut = "cannot delete " + entries[static_cast<size_t>(index)].file.getFileName();
        return false;
    }

    const auto currentName = currentIndex >= 0 && currentIndex < static_cast<int>(entries.size())
                                  ? entries[static_cast<size_t>(currentIndex)].name
                                  : juce::String();
    rescan();
    currentIndex = currentName.isNotEmpty() ? indexOf(currentName) : -1;
    return true;
}

void PresetManager::syncCurrentIndexFromState()
{
    const auto name = processor.apvts.state.getProperty("presetName").toString();
    currentIndex = name.isNotEmpty() ? indexOf(name) : -1;
}

} // namespace tc
