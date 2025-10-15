cmake_minimum_required(VERSION 3.15)

project(Auralis VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(JUCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../JUCE" CACHE PATH "Path to the JUCE framework directory")

if(NOT EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found. Set JUCE_SOURCE_DIR to a local JUCE checkout before configuring the project.")
endif()

add_subdirectory(${JUCE_SOURCE_DIR} JUCE)

juce_add_plugin(Auralis
    COMPANY_NAME "Auralis Labs"
    BUNDLE_ID com.auralis.plugin
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Aurl
    PLUGIN_CODE Aur1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Auralis"
)

target_sources(Auralis
    PRIVATE
        Source/AuralisProcessor.cpp
        Source/AuralisProcessor.h
        Source/AuralisEditor.cpp
        Source/AuralisEditor.h
        Source/Dsp/Oscillators/BasicOscillator.h
        Source/Dsp/Oscillators/Lfo.h
        Source/Dsp/Synth/AuralisVoice.cpp
        Source/Dsp/Synth/AuralisVoice.h
        Source/Dsp/Synth/ParameterState.h
        Source/Param/ParamIDs.h
        Source/Param/ParamTree.cpp
        Source/Param/ParamTree.h
)

target_include_directories(Auralis
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_compile_definitions(Auralis
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(Auralis
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_plugin_client
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_dsp
)
