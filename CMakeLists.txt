cmake_minimum_required(VERSION 3.22)

# Universal Binary for macOS — MUST be set BEFORE project()
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version (universal binaries cannot go below 11.0)")
endif()

project(TransientCreator VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as subdirectory (git submodule)
add_subdirectory(JUCE)

# The ZQ SFX house UI (tokens, LookAndFeel, bound controls, embedded OFL fonts). Must come
# AFTER JUCE. Pinned by tag: bump deliberately, then re-run transient_ui_snapshot and compare.
include(FetchContent)
FetchContent_Declare(zqsfx_ui
    GIT_REPOSITORY https://github.com/themightyzq/zqsfx_ui.git
    GIT_TAG v0.4.0
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(zqsfx_ui)

# Plugin format selection
set(PLUGIN_FORMATS VST3 AU Standalone)

juce_add_plugin(TransientCreator
    # === COMPANY INFO ===
    COMPANY_NAME "ZQ SFX"
    COMPANY_WEBSITE "https://www.zq-sfx.com"
    COMPANY_EMAIL "connect@zq-sfx.com"
    COMPANY_COPYRIGHT "Copyright (c) 2026 ZQ SFX"

    # === UNIQUE IDENTIFIERS ===
    PLUGIN_MANUFACTURER_CODE ZQSF
    PLUGIN_CODE TrCr
    BUNDLE_ID "com.zqsfx.transientcreator"

    # === PLUGIN TYPE ===
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    # === UI SETTINGS ===
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # === BUILD SETTINGS ===
    COPY_PLUGIN_AFTER_BUILD TRUE
    FORMATS ${PLUGIN_FORMATS}

    # === DISPLAY INFO ===
    PRODUCT_NAME "Transient Creator"
    PLUGIN_NAME "Transient Creator"
    PLUGIN_DESCRIPTION "Doppler-inspired transient creation effect"

    # === VST3 CATEGORIES ===
    VST3_CATEGORIES "Fx" "Dynamics" "Tools"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

juce_generate_juce_header(TransientCreator)

# Factory presets (Docs/PRESETS.md), embedded so they travel inside the VST3/AU bundle.
# Each file is the XML of apvts.copyState() with a "presetName" attribute -- see
# Source/Presets/PresetManager.{h,cpp}.
file(GLOB TRANSIENTCREATOR_PRESET_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Presets/*.tcpreset")
juce_add_binary_data(TransientCreatorPresets SOURCES ${TRANSIENTCREATOR_PRESET_FILES})

target_sources(TransientCreator
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters/ParameterLayout.cpp
        Source/DSP/TransientEngine.cpp
        Source/DSP/EnvelopeGenerator.cpp
        Source/DSP/DopplerProcessor.cpp
        Source/UI/LookAndFeel/TransientLookAndFeel.cpp
        Source/UI/Components/EnvelopeVisualizer.cpp
        Source/UI/Components/TransientControls.cpp
        Source/UI/Panels/MainPanel.cpp
        Source/Presets/PresetManager.cpp
)

target_include_directories(TransientCreator
    PRIVATE
        Source
)

target_compile_definitions(TransientCreator
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
)

if(APPLE)
    target_compile_definitions(TransientCreator PUBLIC
        JUCE_JACK=0
        JUCE_ALSA=0
    )
endif()

target_link_libraries(TransientCreator
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        zqsfx::zqsfx_ui
        TransientCreatorPresets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---- UI snapshot tool (headless before/after render gate for the zqsfx_ui house-UI migration)
# pamplejuce pattern: link against the plugin's own shared-code target ("TransientCreator",
# produced by juce_add_plugin above) instead of recompiling PluginProcessor.cpp/PluginEditor.cpp
# a second time. juce_generate_juce_header(TransientCreator)'s generated JuceHeader.h is only
# added to TransientCreator's OWN include path as PRIVATE, so it does not propagate through
# target_link_libraries; it is added back explicitly below via JUCE_GENERATED_SOURCES_DIRECTORY.
juce_add_console_app(transient_ui_snapshot PRODUCT_NAME "transient_ui_snapshot")
target_sources(transient_ui_snapshot PRIVATE tools/ui_snapshot/main.cpp)
target_include_directories(transient_ui_snapshot
    PRIVATE
        Source
        $<TARGET_PROPERTY:TransientCreator,JUCE_GENERATED_SOURCES_DIRECTORY>
)
target_link_libraries(transient_ui_snapshot
    PRIVATE
        TransientCreator
        TransientCreatorPresets
        juce::juce_audio_utils
        juce::juce_dsp
        zqsfx::zqsfx_ui
)

# ---- Preset regression console app: constructs the real processor, loads every factory
# preset via PresetManager and asserts it differs from Init, then round-trips a user
# preset's name (saved to a temp dir, not the real user preset folder) through
# getStateInformation()/setStateInformation(). Same link-against-TransientCreator pattern as
# transient_ui_snapshot above -- guarded the same way as Tests/ below, since the source lives
# in Diversion, not the git mirror.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/PresetManagerTests.cpp")
    juce_add_console_app(transient_preset_tests PRODUCT_NAME "transient_preset_tests")
    target_sources(transient_preset_tests PRIVATE Tests/PresetManagerTests.cpp)
    target_include_directories(transient_preset_tests
        PRIVATE
            Source
            $<TARGET_PROPERTY:TransientCreator,JUCE_GENERATED_SOURCES_DIRECTORY>
    )
    target_link_libraries(transient_preset_tests
        PRIVATE
            TransientCreator
            TransientCreatorPresets
            juce::juce_audio_utils
            juce::juce_dsp
            zqsfx::zqsfx_ui
    )
endif()

# === Unit tests (JUCE UnitTest framework, no external dependency) ===
# Tests/ lives in the authoritative VCS (Diversion), not the minimal git mirror,
# so guard on its presence: the mirror configures fine without it.
option(TRANSIENTCREATOR_BUILD_TESTS "Build the DSP unit-test runner" ON)
if(TRANSIENTCREATOR_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(Tests)
    if(TARGET transient_preset_tests)
        add_test(NAME TransientCreatorPresetTests COMMAND transient_preset_tests)
    endif()
endif()
