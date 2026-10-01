# Tests/CMakeLists.txt — DSP unit tests via JUCE's built-in UnitTest framework.
# No external test dependency: the runner links the DSP sources directly and
# exercises them through their public interfaces.

juce_add_console_app(TransientCreatorTests
    PRODUCT_NAME "TransientCreatorTests")

target_sources(TransientCreatorTests
    PRIVATE
        TestMain.cpp
        EnvelopeGeneratorTests.cpp
        TransientEngineTests.cpp
        ParameterLayoutTests.cpp
        DefaultsTests.cpp
        # Units under test (compiled directly into the test binary)
        ${CMAKE_SOURCE_DIR}/Source/DSP/EnvelopeGenerator.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/TransientEngine.cpp
        ${CMAKE_SOURCE_DIR}/Source/DSP/DopplerProcessor.cpp
        ${CMAKE_SOURCE_DIR}/Source/Parameters/ParameterLayout.cpp)

target_include_directories(TransientCreatorTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}            # TestHelpers.h
        ${CMAKE_SOURCE_DIR}/Source)

target_compile_definitions(TransientCreatorTests
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STANDALONE_APPLICATION=1)

target_link_libraries(TransientCreatorTests
    PRIVATE
        juce::juce_dsp
        juce::juce_audio_basics
        juce::juce_audio_utils      # APVTS / AudioParameterFloat for ParameterLayout tests
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

add_test(NAME TransientCreatorTests COMMAND TransientCreatorTests)
