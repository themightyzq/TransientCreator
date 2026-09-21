cmake_minimum_required(VERSION 3.22)

# Universal Binary for macOS — MUST be set BEFORE project()
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures for macOS")
endif()

project(TransientCreator VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE as subdirectory (git submodule)
add_subdirectory(JUCE)

# Plugin format selection
set(PLUGIN_FORMATS VST3 Standalone)

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
)

juce_generate_juce_header(TransientCreator)

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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# === Unit tests (JUCE UnitTest framework, no external dependency) ===
# Tests/ lives in the authoritative VCS (Diversion), not the minimal git mirror,
# so guard on its presence: the mirror configures fine without it.
option(TRANSIENTCREATOR_BUILD_TESTS "Build the DSP unit-test runner" ON)
if(TRANSIENTCREATOR_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Tests/CMakeLists.txt")
    enable_testing()
    add_subdirectory(Tests)
endif()
