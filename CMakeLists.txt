cmake_minimum_required(VERSION 3.22)

project(MiguelMusicAssistant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MiguelMusicAssistant
    COMPANY_NAME "Miguel Audio"
    BUNDLE_ID "com.miguelaudio.musicassistant"
    PLUGIN_MANUFACTURER_CODE MgAu
    PLUGIN_CODE MmAs
    FORMATS Standalone
    PRODUCT_NAME "Miguel Music Assistant"
    IS_SYNTH FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
)

target_sources(MiguelMusicAssistant PRIVATE
    Source/GraphicEqDisplay.cpp
    Source/GraphicEqDisplay.h
    Source/GrooveEngine.cpp
    Source/GrooveEngine.h
    Source/ImportedSampleList.cpp
    Source/ImportedSampleList.h
    Source/MiguelLookAndFeel.cpp
    Source/MiguelLookAndFeel.h
    Source/MixAnalyzerComponent.cpp
    Source/MixAnalyzerComponent.h
    Source/MusicGenerator.cpp
    Source/MusicGenerator.h
    Source/PianoRollView.cpp
    Source/PianoRollView.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PrecisionRotarySlider.cpp
    Source/PrecisionRotarySlider.h
    Source/SessionState.cpp
    Source/SessionState.h
    Source/SectionEq.cpp
    Source/SectionEq.h
)

if(MSVC)
    target_compile_options(MiguelMusicAssistant PRIVATE /utf-8)
endif()

target_compile_definitions(MiguelMusicAssistant PUBLIC
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
)

target_link_libraries(MiguelMusicAssistant PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
