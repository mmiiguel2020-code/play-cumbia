cmake_minimum_required(VERSION 3.22)

project(MiguelMusicAssistant VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BAJXTER_BUILD_APP "Compilar PICHADAW (app y VST3)" ON)
option(BAJXTER_BUILD_FX "Compilar plugins de efectos" ON)

set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

set(BAJXTER_VST3_DIR "$ENV{USERPROFILE}/Documents/Miguel Music Assistant/VST3")
set(BAJXTER_SHARED_SOURCES
    Source/Shared/FxRack.cpp
    Source/Shared/FxRack.h
    Source/Shared/MiguelLookAndFeel.cpp
    Source/Shared/MiguelLookAndFeel.h
    Source/Shared/PrecisionRotarySlider.cpp
    Source/Shared/PrecisionRotarySlider.h
)

function(bajxter_link target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8)
    endif()
    target_include_directories(${target_name} PRIVATE
        "${CMAKE_SOURCE_DIR}/Source"
        "${CMAKE_SOURCE_DIR}/Source/App"
        "${CMAKE_SOURCE_DIR}/Source/Shared"
        "${CMAKE_SOURCE_DIR}/Source/Modules")
    target_compile_definitions(${target_name} PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_CURL=0
        JUCE_WEB_BROWSER=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(${target_name} PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
endfunction()

function(bajxter_copy_vst3 target_name)
    add_custom_command(TARGET ${target_name}_VST3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${BAJXTER_VST3_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/${target_name}_artefacts/$<CONFIG>/VST3"
            "${BAJXTER_VST3_DIR}"
        COMMENT "Copiando VST3 a Documentos/Miguel Music Assistant/VST3")
endfunction()

if(BAJXTER_BUILD_APP)
    juce_add_plugin(MiguelMusicAssistant
        COMPANY_NAME "Miguel Audio"
        BUNDLE_ID "com.miguelaudio.musicassistant"
        PLUGIN_MANUFACTURER_CODE MgAu
        PLUGIN_CODE MmAs
        FORMATS Standalone VST3
        PRODUCT_NAME "PICHADAW"
        ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/PICHADAW.png"
        IS_SYNTH TRUE
        IS_MIDI_EFFECT FALSE
        NEEDS_MIDI_INPUT TRUE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        VST3_CATEGORIES Instrument
    )
    target_sources(MiguelMusicAssistant PRIVATE
        ${BAJXTER_SHARED_SOURCES}
        Source/App/GraphicEqDisplay.cpp
        Source/App/GraphicEqDisplay.h
        Source/App/GrooveEngine.cpp
        Source/App/GrooveEngine.h
        Source/App/ImportedSampleList.cpp
        Source/App/ImportedSampleList.h
        Source/App/MixAnalyzerComponent.cpp
        Source/App/MixAnalyzerComponent.h
        Source/App/MusicGenerator.cpp
        Source/App/MusicGenerator.h
        Source/App/PianoRollView.cpp
        Source/App/PianoRollView.h
        Source/App/PluginEditor.cpp
        Source/App/PluginEditor.h
        Source/App/PluginProcessor.cpp
        Source/App/PluginProcessor.h
        Source/App/ReferencePiano.cpp
        Source/App/ReferencePiano.h
        Source/App/RackPanel.cpp
        Source/App/RackPanel.h
        Source/App/SessionState.cpp
        Source/App/SessionState.h
        Source/App/SectionEq.cpp
        Source/App/SectionEq.h
    )
    bajxter_link(MiguelMusicAssistant)
    bajxter_copy_vst3(MiguelMusicAssistant)
endif()

function(bajxter_fx target_name product code slot_index)
    juce_add_plugin(${target_name}
        COMPANY_NAME "Miguel Audio"
        BUNDLE_ID "com.miguelaudio.${target_name}"
        PLUGIN_MANUFACTURER_CODE MgAu
        PLUGIN_CODE ${code}
        FORMATS VST3
        PRODUCT_NAME ${product}
        IS_SYNTH FALSE
        IS_MIDI_EFFECT FALSE
        NEEDS_MIDI_INPUT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES Fx
    )
    target_sources(${target_name} PRIVATE
        ${BAJXTER_SHARED_SOURCES}
        Source/Modules/BajxterFxProcessor.cpp
        Source/Modules/BajxterFxProcessor.h
        Source/Modules/BajxterFxEditor.cpp
        Source/Modules/BajxterFxEditor.h
    )
    bajxter_link(${target_name})
    target_compile_definitions(${target_name} PUBLIC BAJXTER_FX_SLOT=${slot_index})
    bajxter_copy_vst3(${target_name})
endfunction()

if(BAJXTER_BUILD_FX)
    bajxter_fx(BajxterFxHp        "Bajxter Filtro HP"   BxHp 0)
    bajxter_fx(BajxterFxLp        "Bajxter Filtro LP"   BxLp 1)
    bajxter_fx(BajxterFxComp      "Bajxter Compresor"   BxCp 2)
    bajxter_fx(BajxterFxExciter   "Bajxter Excitador"   BxEx 3)
    bajxter_fx(BajxterFxDoubler   "Bajxter Duplicador"  BxDb 4)
    bajxter_fx(BajxterFxDist      "Bajxter Distorsion"  BxDs 5)
    bajxter_fx(BajxterFxDelay     "Bajxter Delay"       BxDl 6)
    bajxter_fx(BajxterFxEfecto    "Bajxter Efecto"      BxEf 7)
    bajxter_fx(BajxterFxVolume    "Bajxter Volumen"     BxVl 8)
    bajxter_fx(BajxterFxVelocity  "Bajxter Velocity"    BxVe 9)

    juce_add_plugin(BajxterTuner
        COMPANY_NAME "Miguel Audio"
        BUNDLE_ID "com.miguelaudio.bajxtertuner"
        PLUGIN_MANUFACTURER_CODE MgAu
        PLUGIN_CODE BxTn
        FORMATS VST3
        PRODUCT_NAME "Bajxter Afinador"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES Fx
    )
    target_sources(BajxterTuner PRIVATE
        Source/Shared/MiguelLookAndFeel.cpp
        Source/Shared/MiguelLookAndFeel.h
        Source/Modules/BajxterTunerProcessor.cpp
        Source/Modules/BajxterTunerProcessor.h
        Source/Modules/BajxterTunerEditor.cpp
        Source/Modules/BajxterTunerEditor.h
    )
    bajxter_link(BajxterTuner)
    bajxter_copy_vst3(BajxterTuner)

    juce_add_plugin(BajxterEq
        COMPANY_NAME "Miguel Audio"
        BUNDLE_ID "com.miguelaudio.bajxtereq"
        PLUGIN_MANUFACTURER_CODE MgAu
        PLUGIN_CODE BxEq
        FORMATS VST3
        PRODUCT_NAME "Bajxter EQ"
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        VST3_CATEGORIES Fx
    )
    target_sources(BajxterEq PRIVATE
        Source/Shared/MiguelLookAndFeel.cpp
        Source/Shared/MiguelLookAndFeel.h
        Source/Shared/PrecisionRotarySlider.cpp
        Source/Shared/PrecisionRotarySlider.h
        Source/Modules/BajxterEqProcessor.cpp
        Source/Modules/BajxterEqProcessor.h
        Source/Modules/BajxterEqEditor.cpp
        Source/Modules/BajxterEqEditor.h
    )
    bajxter_link(BajxterEq)
    bajxter_copy_vst3(BajxterEq)
endif()
