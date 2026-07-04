cmake_minimum_required(VERSION 3.22)
project(Dissolution VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Dissolution
    VERSION                     "0.1.0"
    COMPANY_NAME                "WayneAudio"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    PLUGIN_MANUFACTURER_CODE    "WnAu"
    PLUGIN_CODE                 "Dsln"
    BUNDLE_ID                   "com.wayneaudio.dissolution"
    FORMATS                     VST3 AU Standalone
    PRODUCT_NAME                "Dissolution"
    MICROPHONE_PERMISSION_ENABLED TRUE
)

juce_generate_juce_header(Dissolution)

target_sources(Dissolution PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/dsp/FuzzEngine.cpp
    src/dsp/FeedbackMatrix.cpp
    src/dsp/BitCrusher.cpp
    src/dsp/ShimmerReverb.cpp
    src/dsp/SpectralFreeze.cpp
)

target_include_directories(Dissolution PRIVATE src)

target_compile_definitions(Dissolution PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Dissolution PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
