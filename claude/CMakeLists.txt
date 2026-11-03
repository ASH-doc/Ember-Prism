cmake_minimum_required(VERSION 3.22)

# Static C++ runtime: the plugin and the app run on PCs without the Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(AETHERDUO VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(DUO_BUILD_PLUGIN "Build the VST3 + Standalone" ON)
option(DUO_BUILD_TESTS  "Build the offline self-test and the UI snapshot tool" ON)

# ---- JUCE (bundled in third_party, no network needed) ----------------------------------------------------
add_subdirectory(third_party/JUCE)

set(DUO_DEFINITIONS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_MODAL_LOOPS_PERMITTED=1)

# ---- DSP: two completely separate engines + the host's mixer/master (no UI in these files) ---------------
set(DUO_DSP_SOURCES
    Source/SynthA/SA_Engine.cpp
    Source/SynthA/SA_Presets.cpp
    Source/SynthB/SB_Wavetables.cpp
    Source/SynthB/SB_Engine.cpp
    Source/SynthB/SB_Presets.cpp
    Source/Host/HostEngine.cpp)

set(_existing "")
foreach(f ${DUO_DSP_SOURCES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${f}")
        list(APPEND _existing ${f})
    endif()
endforeach()
set(DUO_DSP_SOURCES ${_existing})

file(GLOB DUO_UI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/SynthA/ui/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/SynthB/ui/*.cpp")

if(DUO_BUILD_PLUGIN)
    juce_add_plugin(AetherDuo
        COMPANY_NAME                "Aether Audio"
        BUNDLE_ID                   com.aetheraudio.aetherduo
        PRODUCT_NAME                "AETHER DUO"
        PLUGIN_NAME                 "AETHER DUO"
        DESCRIPTION                 "EMBER + PRISM two-synth instrument"
        IS_SYNTH                    TRUE
        NEEDS_MIDI_INPUT            TRUE
        NEEDS_MIDI_OUTPUT           FALSE
        IS_MIDI_EFFECT              FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS TRUE
        COPY_PLUGIN_AFTER_BUILD     FALSE
        PLUGIN_MANUFACTURER_CODE    Aeau
        PLUGIN_CODE                 Aedu
        FORMATS                     VST3 Standalone
        VST3_CATEGORIES             Instrument Synth)

    target_sources(AetherDuo PRIVATE ${DUO_DSP_SOURCES} ${DUO_UI_SOURCES} Source/PluginProcessor.cpp)
    target_include_directories(AetherDuo PRIVATE Source)
    target_compile_definitions(AetherDuo PUBLIC ${DUO_DEFINITIONS} JUCE_VST3_EMULATE_MIDI_CC_WITH_PARAMETERS=0)
    target_link_libraries(AetherDuo
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
    if(MSVC)
        target_compile_options(AetherDuo PRIVATE /utf-8 /MP /bigobj)
    endif()
endif()

if(DUO_BUILD_TESTS)
    juce_add_console_app(DuoSelfTest PRODUCT_NAME "DuoSelfTest")
    target_sources(DuoSelfTest PRIVATE ${DUO_DSP_SOURCES} Tests/SelfTest.cpp)
    target_include_directories(DuoSelfTest PRIVATE Source)
    target_compile_definitions(DuoSelfTest PRIVATE ${DUO_DEFINITIONS})
    target_link_libraries(DuoSelfTest PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
    if(MSVC)
        target_compile_options(DuoSelfTest PRIVATE /utf-8 /MP /bigobj)
    endif()

    if(DUO_BUILD_PLUGIN)
        juce_add_console_app(DuoUiShot PRODUCT_NAME "DuoUiShot")
        target_sources(DuoUiShot PRIVATE ${DUO_DSP_SOURCES} ${DUO_UI_SOURCES} Source/PluginProcessor.cpp Tests/UiShot.cpp)
        target_include_directories(DuoUiShot PRIVATE Source)
        target_compile_definitions(DuoUiShot PRIVATE ${DUO_DEFINITIONS})
        target_link_libraries(DuoUiShot PRIVATE juce::juce_audio_utils juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
        if(MSVC)
            target_compile_options(DuoUiShot PRIVATE /utf-8 /MP /bigobj)
        endif()
        juce_add_console_app(DuoPluginTest PRODUCT_NAME "DuoPluginTest")
        target_sources(DuoPluginTest PRIVATE ${DUO_DSP_SOURCES} ${DUO_UI_SOURCES} Source/PluginProcessor.cpp Tests/PluginTest.cpp)
        target_include_directories(DuoPluginTest PRIVATE Source)
        target_compile_definitions(DuoPluginTest PRIVATE ${DUO_DEFINITIONS} JUCE_PLUGINHOST_VST3=1)
        target_link_libraries(DuoPluginTest PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp PUBLIC juce::juce_recommended_config_flags)
        if(MSVC)
            target_compile_options(DuoPluginTest PRIVATE /utf-8 /MP /bigobj)
        endif()
    endif()
endif()
