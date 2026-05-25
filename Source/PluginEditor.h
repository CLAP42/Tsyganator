#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <array>
#include "PluginProcessor.h"

//==============================================================================
// TsyganatorLookAndFeel
//==============================================================================
class TsyganatorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    struct ColorScheme
    {
        juce::Colour primary;
        juce::Colour secondary;
        juce::Colour accent;
        juce::Colour background;
        juce::Colour text;
        juce::Colour buttonFill;
        juce::Colour knobOutline;
    };

    TsyganatorLookAndFeel();
    ~TsyganatorLookAndFeel() override = default;

    void setMode(bool isBelgian);

    // Custom rotary slider (3D metallic knobs with indicator)
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                         float sliderPosProportional, float rotaryStartAngle,
                         float rotaryEndAngle, juce::Slider&) override;

    // Custom linear slider (vertical faders with LED segments)
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         const juce::Slider::SliderStyle, juce::Slider&) override;

    // Custom buttons
    void drawButtonBackground(juce::Graphics&, juce::Button&,
                             const juce::Colour& backgroundColour,
                             bool isMouseOverButton, bool isButtonDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
                       bool isMouseOverButton, bool isButtonDown) override;

    // Custom combo box
    void drawComboBox(juce::Graphics&, int width, int height,
                     bool isButtonDown, int buttonX, int buttonY,
                     int buttonW, int buttonH, juce::ComboBox&) override;

    void drawPopupMenuItem(juce::Graphics&, const juce::Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted,
                          bool isTicked, bool hasSubMenu, const juce::String& text,
                          const juce::String& shortcutKeyText, const juce::Drawable* icon,
                          const juce::Colour* textColour) override;

    // Custom popup menu background
    void drawPopupMenuBackground(juce::Graphics&, int width, int height) override;

    // Constrain popup menu item size to prevent giant dropdowns
    void getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                   int standardMenuItemHeight, int& idealWidth,
                                   int& idealHeight) override;

    // Cleanup L: setImages() removed — the LookAndFeel no longer stores
    // image assets. Knob / fader / segment renderers went fully procedural.

    // Embedded font resolution — serves Outfit and JetBrains Mono from BinaryData
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font& font) override;

private:
    ColorScheme belgianScheme;
    ColorScheme italianScheme;
    ColorScheme currentScheme;

    // Cached typefaces loaded from BinaryData
    juce::Typeface::Ptr outfitBoldTypeface;
    juce::Typeface::Ptr outfitExtraBoldTypeface;
    juce::Typeface::Ptr jetBrainsMonoBoldTypeface;
};

//==============================================================================
// TsyganatorEditor
//==============================================================================
class TsyganatorEditor : public juce::AudioProcessorEditor,
                        public TsyganatorProcessor::ModeListener,
                        public juce::Timer
{
public:
    explicit TsyganatorEditor(TsyganatorProcessor&);
    ~TsyganatorEditor() override;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void modeChanged(TsyganatorProcessor::SynthMode newMode) override;
    void timerCallback() override;

    // P36c v2: CRITICAL — force a FULL repaint of the editor whenever it
    // becomes visible or is re-parented. Multiple hooks because different
    // hosts (Ableton, Logic, FL Studio…) trigger these callbacks at
    // different points in their plugin-window lifecycle.
    void visibilityChanged() override;
    void parentHierarchyChanged() override;

private:
    TsyganatorProcessor& processor;
    TsyganatorLookAndFeel lookAndFeel;

    // Image assets loaded from BinaryData.
    // Cleanup L: removed bg*, knobFilm*, fader*, ledSeg*, mascot* — they
    // were loaded but never drawn (UI is fully procedural).
    juce::Image ledOnBelgian, ledOffBelgian, ledOnItalian, ledOffItalian;
    juce::Image logoBelgian, logoItalian;

    // P33: Acid Smiley loaded from the user-supplied SVG so the Belgian
    // mascot face is byte-identical to the reference (pixel-perfect).
    // Stars and the EU-blue circle are still procedural so the orbiting
    // animation runs every frame; only the face is the SVG drawable.
    std::unique_ptr<juce::Drawable> acidSmileyDrawable;

    // ========== OSC1 Section ==========
    juce::Label osc1Label;

    juce::Slider sawLevelSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sawLevelAttach;
    juce::Label sawLevelLabel;

    juce::Slider pulseLevelSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulseLevelAttach;
    juce::Label pulseLevelLabel;

    juce::Slider triangleLevelSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> triangleLevelAttach;
    juce::Label triangleLevelLabel;

    juce::Slider subLevelSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subLevelAttach;
    juce::Label subLevelLabel;

    juce::Slider noiseLevelSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseLevelAttach;
    juce::Label noiseLevelLabel;

    juce::Slider pulseWidthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pulseWidthAttach;
    juce::Label pulseWidthLabel;

    juce::Slider osc1VolumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc1VolumeAttach;
    juce::Label osc1VolumeLabel;

    // ========== OSC2 Section ==========
    juce::Label osc2Label;

    juce::Slider osc2SawSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2SawAttach;
    juce::Label osc2SawLabel;

    juce::Slider osc2PulseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2PulseAttach;
    juce::Label osc2PulseLabel;

    juce::Slider osc2TriangleSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2TriangleAttach;
    juce::Label osc2TriangleLabel;

    juce::Slider osc2PWSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2PWAttach;
    juce::Label osc2PWLabel;

    juce::Slider osc2FineSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2FineAttach;
    juce::Label osc2FineLabel;

    juce::Slider osc2VolumeSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> osc2VolumeAttach;
    juce::Label osc2VolumeLabel;

    juce::ComboBox osc2OctaveCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> osc2OctaveAttach;
    juce::Label osc2OctaveLabel;

    // ========== Filter Section ==========
    juce::Label filterLabel;

    juce::Slider cutoffSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttach;
    juce::Label cutoffLabel;

    juce::Slider resonanceSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttach;
    juce::Label resonanceLabel;

    juce::Slider filterEnvAmountSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvAmountAttach;
    juce::Label filterEnvAmountLabel;

    // ========== Filter ADSR ==========
    juce::Label filterADSRLabel;

    juce::Slider filterAttackSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterAttackAttach;
    juce::Label filterAttackLabel;

    juce::Slider filterDecaySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterDecayAttach;
    juce::Label filterDecayLabel;

    juce::Slider filterSustainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterSustainAttach;
    juce::Label filterSustainLabel;

    juce::Slider filterReleaseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterReleaseAttach;
    juce::Label filterReleaseLabel;

    // ========== Amp ADSR ==========
    juce::Label ampADSRLabel;

    juce::Slider ampAttackSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampAttackAttach;
    juce::Label ampAttackLabel;

    juce::Slider ampDecaySlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampDecayAttach;
    juce::Label ampDecayLabel;

    juce::Slider ampSustainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampSustainAttach;
    juce::Label ampSustainLabel;

    juce::Slider ampReleaseSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampReleaseAttach;
    juce::Label ampReleaseLabel;

    // ========== Performance Section ==========
    juce::Label performanceLabel;

    juce::ComboBox unisonModeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> unisonModeAttach;
    juce::Label unisonModeLabel;

    juce::Slider unisonDetuneSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> unisonDetuneAttach;
    juce::Label unisonDetuneLabel;

    juce::Slider keyTrackingSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> keyTrackingAttach;
    juce::Label keyTrackingLabel;

    juce::Slider portamentoSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> portamentoAttach;
    juce::Label portamentoLabel;

    // ========== Effects Section ==========
    juce::Label effectsLabel;

    juce::Slider globalFineTuneSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalFineTuneAttach;
    juce::Label globalFineTuneLabel;

    // ========== Vintage Section ==========
    juce::TextButton vintageButton;
    juce::Slider vintageAmountSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> vintageAmountAttach;
    juce::Label vintageAmountLabel;

    // ========== LFO Section ==========
    juce::Label lfoLabel;

    juce::Slider lfoRateSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoRateAttach;
    juce::Label lfoRateLabel;

    juce::Slider lfoDepthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfoDepthAttach;
    juce::Label lfoDepthLabel;

    juce::ComboBox lfoWaveformCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoWaveformAttach;
    juce::Label lfoWaveformLabel;

    juce::ComboBox lfoDestinationCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoDestinationAttach;
    juce::Label lfoDestinationLabel;

    // LFO Sync
    juce::TextButton lfoSyncButton;  // Toggle Free/Sync
    juce::ComboBox lfoSyncRateCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfoSyncRateAttach;
    juce::Label lfoSyncLabel;

    // ========== Chorus Section ==========
    juce::Label chorusLabel;

    juce::ComboBox chorusModeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> chorusModeAttach;
    juce::Label chorusModeLabel;

    // Chorus toggle buttons (visual match for HTML preview toggle switches)
    juce::TextButton chorusOffBtn, chorusIBtn, chorusIIBtn, chorusIPlusIIBtn;

    // ========== Master Section ==========
    juce::Label masterLabel;

    juce::Slider masterGainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttach;
    juce::Label masterGainLabel;
    juce::Label masterDbLabel;

    // ========== Sequencer Section ==========
    juce::Label sequencerLabel;

    juce::Slider seqNumStepsSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seqNumStepsAttach;
    juce::Label seqNumStepsLabel;

    juce::Slider seqSwingSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seqSwingAttach;
    juce::Label seqSwingLabel;

    juce::Slider seqGateLengthSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> seqGateLengthAttach;
    juce::Label seqGateLengthLabel;

    // Sequencer action buttons
    juce::ComboBox   seqPatternCombo;   // Pattern preset selector
    juce::TextButton seqRandButton;
    juce::TextButton seqClearButton;
    juce::TextButton seqGlideButton;
    juce::TextButton seqAccentButton;
    juce::TextButton seqNotePlusButton;
    juce::TextButton seqNoteMinusButton;
    juce::TextButton seqVelPlusButton;
    juce::TextButton seqVelMinusButton;

    // Sequencer step display
    static constexpr int maxSequencerSteps = 16;
    std::array<juce::TextButton, maxSequencerSteps> seqStepButtons;

    juce::TextButton stepPlusButton;
    juce::TextButton stepMinusButton;

    // ========== Arpeggiator Section ==========
    juce::Label arpLabel;

    juce::ComboBox arpRateCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpRateAttach;
    juce::Label arpRateLabel;

    // Pattern selector (Up / Down / Up-Down / Random) — added in rebrand batch.
    juce::ComboBox arpModeCombo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> arpModeAttach;
    juce::Label arpModeLabel;

    // ========== Mode & Play Mode ==========
    juce::TextButton modeButton;  // Toggle Belgian/Italian

    juce::TextButton playOffButton;
    juce::TextButton playArpButton;
    juce::TextButton playSeqSynthButton;
    juce::TextButton playSeqSampleButton;

    // ========== Additional Buttons ==========
    juce::TextButton unisonButton;
    juce::TextButton tsyganizeButton;
    juce::TextButton initButton;
    juce::TextButton saveButton;
    juce::TextButton loadSampleButton;

    // ========== Presets ==========
    juce::Label presetLabel;
    juce::ComboBox presetCombo;
    juce::TextButton presetPrevButton;
    juce::TextButton presetNextButton;

    // ========== Status & Indicators ==========
    juce::Label statusLabel;
    juce::Label midiLedLabel;

    // ========== File Chooser ==========
    std::unique_ptr<juce::FileChooser> fileChooser;

    // ========== Tooltip Window ==========
    // Required to display the setTooltip() texts on hover. ~700 ms delay
    // before showing so it doesn't get in the way of casual use.
    juce::TooltipWindow tooltipWindow { this, 700 };

    // ========== Layout Helper Methods ==========
    void layoutRow1();
    void layoutRow2();
    void layoutRow3();
    void layoutRow4();

    void syncMode();
    void updateMasterDbLabel();

    // ========== Button Callbacks ==========
    void buttonClicked(juce::Button* button);
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged);

    // ========== State Tracking ==========
    bool midiLedOn = false;
    float mascotScale = 1.0f;
    int lastDisplayedStep = -1;

    // P43 A: warm-up full-repaint window after the editor is constructed.
    // The first ~200 timer ticks (~3 s) we issue a FULL repaint every
    // 6 ticks (~90 ms) to flush any stale Cocoa NSView layer content —
    // specifically targets AU-on-Ableton-12 ghost-overlay regression.
    int warmupTicksRemaining = 200;

    // Dynamic effect state
    float tsyganizeFlash = 0.0f;
    float stepEditFlash = 0.0f;   // Flash when N+/N-/V+/V-/Accent/Glide modifies a step
    int   stepEditFlashIdx = -1;  // Which step index is flashing
    int   selectedStep = 0;       // User-selected step for editing (click to select)
    float actionFlash = 0.0f;     // Flash glow on action buttons (Rand, Clr, etc.)
    juce::Component* actionFlashBtn = nullptr;  // Which button is flashing
    float discoAngle = 0.0f;     // Italian disco ball Y-axis rotation
    float smileyAngle = 0.0f;    // Belgian star orbit rotation
    float neonPulsePhase = 0.0f; // Mode button neon pulse animation (0-2π)

    // SVG-based acid smiley (pixel-perfect from reference SVG)
    std::unique_ptr<juce::Drawable> smileyDrawable;

    // 3D disco ball renderer (Italian mode mascot)
    void draw3DDiscoBall(juce::Graphics& g, float cx, float cy, float radius, float angle);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TsyganatorEditor)
};
