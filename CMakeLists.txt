cmake_minimum_required(VERSION 3.22)

# ─────────────────────────────────────────────────────────────────────────────
# Platform-specific settings (BEFORE project() so they apply to JUCE too)
# ─────────────────────────────────────────────────────────────────────────────
if(APPLE)
    # macOS 26 Tahoe a supprimé des APIs que JUCE utilise encore.
    # On cible macOS 14 pour que le compilateur les accepte.
    # Le plugin marchera quand même sur macOS 26.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum macOS version" FORCE)
endif()

project(Tsyganator VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────────────────────────────────────
# Per-platform plugin formats
#   macOS   → VST3 + AU + Standalone
#   Windows → VST3 + Standalone  (AU est macOS-only)
#   Linux   → VST3 + Standalone
# ─────────────────────────────────────────────────────────────────────────────
if(APPLE)
    set(TSYG_PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(TSYG_PLUGIN_FORMATS VST3 Standalone)
endif()

# ─────────────────────────────────────────────────────────────────────────────
# MSVC-specific compile options (Windows / Visual Studio)
# ─────────────────────────────────────────────────────────────────────────────
if(MSVC)
    # /utf-8        → source files are UTF-8 (matches non-ASCII chars in our
    #                  comments and string literals; sinon MSVC chiale en C4828)
    # /MP           → parallel compilation (utilise tous les cœurs)
    # /bigobj       → JUCE/template-heavy code dépasse la limite d'objets standard
    add_compile_options(/utf-8 /MP /bigobj)
endif()

# ---- JUCE ----
# JUCE is fetched automatically from GitHub. No manual install needed.
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    # P36c v7: bumped 8.0.4 → 8.0.10 to pick up 6 patch releases — 8.0.7
    # in particular fixes "a crash in Ableton when closing a plug-in window"
    # (directly relevant to our reopen-corruption bug).
    # P41 (QA round 2): bumped 8.0.10 → 8.0.12 (latest stable, 16 Dec 2025).
    # 8.0.11 adds PopupMenu visibility fixes + Direct2D fixes; 8.0.12 fixes
    # some MIDI device names. Tiny upgrade, zero risk.
    # NOT moving to JUCE 9 yet — only available as a preview on the juce9
    # branch (announced 2026-05-21), no tagged release, AudioProcessor v2
    # rework still in progress. Software renderer 2-4× speedup is tempting
    # but the preview status is too risky for a release branch.
    GIT_TAG        8.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ---- Plugin target ----
juce_add_plugin(Tsyganator
    COMPANY_NAME           "TsyganatorAudio"
    BUNDLE_ID              "com.tsyganatoraudio.tsyganator"
    PLUGIN_MANUFACTURER_CODE Tsyg
    PLUGIN_CODE             Tsyn
    FORMATS                 ${TSYG_PLUGIN_FORMATS}
    PRODUCT_NAME            "Tsyganator"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    # Always FALSE: user copies the built artefacts manually.
    #   macOS    → build/Tsyganator_artefacts/Release/{VST3,AU,Standalone}/
    #              into ~/Library/Audio/Plug-Ins/{VST3,Components}
    #   Windows  → build\Tsyganator_artefacts\Release\VST3\Tsyganator.vst3
    #              into %CommonProgramFiles%\VST3\
    # Never auto-installed by the build step.
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# ---- Source files ----
target_sources(Tsyganator
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# ---- Include directories ----
target_include_directories(Tsyganator
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

# ---- Binary Data (Assets) ----
# Cleanup L (2026-05-24): removed 11 assets that were loaded but never
# drawn — UI went full-procedural (knobs, faders, mascots, panels). Kept:
#   - logo_belgian/italian.png  → drawn in top bar
#   - led_on/off_belgian/italian.png  → drawn in step-sequencer LEDs
#   - acid_smiley.svg  → Belgian mascot Drawable
#   - 3 TTF fonts  → typography
juce_add_binary_data(TsyganatorData
    SOURCES
        Resources/led_on_belgian.png
        Resources/led_off_belgian.png
        Resources/led_on_italian.png
        Resources/led_off_italian.png
        Resources/logo_belgian.png
        Resources/logo_italian.png
        Resources/acid_smiley.svg
        Resources/Outfit-Bold.ttf
        Resources/Outfit-ExtraBold.ttf
        Resources/JetBrainsMono-Bold.ttf
)

# ---- JUCE modules ----
target_link_libraries(Tsyganator
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_gui_extra
        juce::juce_dsp
        TsyganatorData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ---- Compile definitions ----
target_compile_definitions(Tsyganator
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)
