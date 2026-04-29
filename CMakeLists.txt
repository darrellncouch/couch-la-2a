cmake_minimum_required(VERSION 3.22)
project(CouchLA2A VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Fetch JUCE ────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.7
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(CouchLA2A
    COMPANY_NAME                "Couch"
    PLUGIN_MANUFACTURER_CODE    Couc
    PLUGIN_CODE                 CLA2
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Couch LA-2A"
    BUNDLE_ID                   "com.couch.couchla2a"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Fx" "Dynamics"
)

juce_generate_juce_header(CouchLA2A)

# ── Binary data (images embedded directly in the binary) ─────────────────────
juce_add_binary_data(CouchLA2A_BinaryData SOURCES
    Resources/knob_filmstrip.png
    Resources/panel_metal.png
    Resources/toggle_up.png
    Resources/toggle_down.png
    Resources/window.png
)

# ── Sources ───────────────────────────────────────────────────────────────────
target_sources(CouchLA2A PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Dsp/OptoCompressor.cpp
)

target_include_directories(CouchLA2A PRIVATE Source)

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(CouchLA2A PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(CouchLA2A
    PRIVATE
        CouchLA2A_BinaryData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
