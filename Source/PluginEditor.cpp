#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "BinaryData.h"
#include <vector>
#include <algorithm>
#include <cmath>

//==============================================================================
// TsyganatorLookAndFeel Implementation
//==============================================================================

TsyganatorLookAndFeel::TsyganatorLookAndFeel()
{
    // ============================================================
    // P34 — POSTER-INSPIRED VINTAGE PALETTES
    // ============================================================
    // Inspired by the DJ TSYGAN posters: warm gold yellow + royal
    // navy for Belgian; warm dark plum + dusty rose for Italian.
    // Less aggressive than the previous saturated colours; all
    // blues and yellows in Belgian share the same gold/navy family,
    // and Italian uses a single warm dark + a single dusty rose.
    // ============================================================

    // Belgian Mode (HARD BELGIAN MODE) — vintage warm gold + royal navy
    //   Yellow family: 0xFFE8C928 (body), 0xFFEFD03A (accent fill)
    //   Navy family:   0xFF1E3F8C (royal navy), 0xFF142C70 (deeper)
    belgianScheme.primary    = juce::Colour(0xFFE8C928);   // Warm gold yellow body
    belgianScheme.secondary  = juce::Colour(0xFFD2B324);   // Deeper gold
    belgianScheme.accent     = juce::Colour(0xFF1E3F8C);   // Royal navy
    belgianScheme.background = juce::Colour(0xFF142C70);   // Deeper royal navy (frame)
    belgianScheme.text       = juce::Colour(0xFF1E3F8C);   // Royal navy text on gold
    belgianScheme.buttonFill = juce::Colour(0xFFE6CB30);   // Mid gold (button surface)
    belgianScheme.knobOutline= juce::Colour(0xFF0E1F50);   // Very dark navy (knob/fader thumb)

    // Italian Mode (SAD ITALIAN MODE) — vintage warm plum + dusty rose
    //   Plum family: 0xFF1A0F2C (body), 0xFF422156 (card)
    //   Rose family: 0xFFC85E92 (accent), 0xFFE8B0C8 (light text)
    italianScheme.primary    = juce::Colour(0xFFE8B0C8);   // Warm rose text
    italianScheme.secondary  = juce::Colour(0xFFC85E92);   // Dusty rose accent
    italianScheme.accent     = juce::Colour(0xFFC85E92);   // Dusty rose
    italianScheme.background = juce::Colour(0xFF1A0F2C);   // Warm dark plum (frame)
    italianScheme.text       = juce::Colour(0xFFE8B0C8);   // Warm rose text
    italianScheme.buttonFill = juce::Colour(0xFF5A3478);   // Mid-plum (raised button surface)
    italianScheme.knobOutline= juce::Colour(0xFFC8A8E0);   // Light plum-lavender fader thumb

    currentScheme = belgianScheme;

    // Load embedded fonts from BinaryData
    outfitBoldTypeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::OutfitBold_ttf, BinaryData::OutfitBold_ttfSize);
    outfitExtraBoldTypeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::OutfitExtraBold_ttf, BinaryData::OutfitExtraBold_ttfSize);
    jetBrainsMonoBoldTypeface = juce::Typeface::createSystemTypefaceFor(
        BinaryData::JetBrainsMonoBold_ttf, BinaryData::JetBrainsMonoBold_ttfSize);
}

void TsyganatorLookAndFeel::setMode(bool isBelgian)
{
    currentScheme = isBelgian ? belgianScheme : italianScheme;
}

// Cleanup L: TsyganatorLookAndFeel::setImages() removed — the knob /
// fader / segment PNGs it stored were never read by drawRotarySlider /
// drawLinearSlider (both gone fully procedural). The method and its
// member images were pure dead state.

void TsyganatorLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPosProportional, float rotaryStartAngle,
                                             float rotaryEndAngle, juce::Slider& slider)
{
    // P25 — Photoreal multi-pass knob renderer.
    //
    // Layer order (bottom → top, like a real knob would build up under light):
    //  0. Drop shadow on the panel (ambient occlusion)
    //  1. Outer bezel ring (chrome / brushed metal)
    //  2. Knob body — radial gradient with off-centre highlight (key light)
    //  3. Subtle concentric rings (lathe-machined texture)
    //  4. Rim shadow (the slight darkening at the knob's silhouette)
    //  5. Specular highlight (the bright catchlight on the top-left)
    //  6. Top centre cap (the small concentric bump)
    //  7. Value arc (luminous ring outside the knob)
    //  8. Tick marks (sérigraphie)
    //  9. Indicator line (with cast shadow + main stroke + white core)
    // 10. Value readout text
    //
    // Each layer is theme-aware. Belgian = navy body / yellow indicator.
    // Italian = dark purple body / pink indicator.
    const bool isBelgian = (currentScheme.primary == belgianScheme.primary);
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 4.0f;
    const float toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Theme palette — P34: aligned with the new vintage poster palette
    // (Belgian = royal navy family, Italian = warm plum family).
    const auto bodyTop   = isBelgian ? juce::Colour(0xFF2A4FA0) : juce::Colour(0xFF3A1F52);
    const auto bodyMid   = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFF291340);
    const auto bodyBot   = isBelgian ? juce::Colour(0xFF0E1F50) : juce::Colour(0xFF120822);
    const auto bezelHi   = isBelgian ? juce::Colour(0xFF7B92D6) : juce::Colour(0xFF8068A8);
    const auto bezelLo   = isBelgian ? juce::Colour(0xFF142C70) : juce::Colour(0xFF1F0E30);
    const auto arcCol    = isBelgian ? juce::Colour(0xFFEFD03A) : currentScheme.accent;
    const auto indCol    = isBelgian ? juce::Colour(0xFFEFD03A) : currentScheme.accent;
    const auto rimShadow = juce::Colours::black.withAlpha(0.45f);
    const auto centreText= isBelgian ? juce::Colour(0xFFEFD03A) : juce::Colour(0xFFE8B0C8);

    // 0. Ambient occlusion — soft blob shadow under the knob ----------------
    {
        const float aoR = radius + 1.0f;
        juce::ColourGradient aoGrad(juce::Colours::black.withAlpha(0.45f), cx, cy + radius * 0.15f,
                                     juce::Colours::black.withAlpha(0.0f),  cx, cy + radius * 1.10f,
                                     true);
        g.setGradientFill(aoGrad);
        g.fillEllipse(cx - aoR, cy - aoR + 1.5f, aoR * 2.0f, aoR * 2.0f);
    }

    // 1. Outer bezel ring — vertical gradient highlight at the top, dark at the bottom
    const float bezelR = radius - 0.5f;
    {
        juce::ColourGradient bz(bezelHi, cx, cy - bezelR,
                                bezelLo, cx, cy + bezelR, false);
        bz.addColour(0.35, bezelHi.withMultipliedBrightness(0.85f));
        bz.addColour(0.65, bezelLo.brighter(0.10f));
        g.setGradientFill(bz);
        g.fillEllipse(cx - bezelR, cy - bezelR, bezelR * 2.0f, bezelR * 2.0f);

        // Crisp 1px highlight on the very top edge (catches light)
        g.setColour(juce::Colours::white.withAlpha(0.16f));
        g.drawEllipse(cx - bezelR + 0.5f, cy - bezelR + 0.5f,
                      (bezelR - 0.5f) * 2.0f, (bezelR - 0.5f) * 2.0f, 0.8f);
    }

    // 2. Inner knob body — diagonal radial gradient (key light top-left)
    const float bodyR = bezelR - 3.0f;
    {
        // Radial gradient with the highlight centred top-left of the body
        const float lightX = cx - bodyR * 0.35f;
        const float lightY = cy - bodyR * 0.45f;
        juce::ColourGradient body(bodyTop,  lightX, lightY,
                                  bodyBot,  cx + bodyR * 0.9f, cy + bodyR * 0.9f,
                                  true);
        body.addColour(0.55, bodyMid);
        g.setGradientFill(body);
        g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
    }

    // 3. Concentric rings — very subtle lathe-machined texture
    {
        g.setColour(juce::Colours::white.withAlpha(0.025f));
        for (int i = 1; i <= 3; ++i)
        {
            const float rr = bodyR * (0.35f + i * 0.18f);
            g.drawEllipse(cx - rr, cy - rr, rr * 2.0f, rr * 2.0f, 0.5f);
        }
    }

    // 4. Rim shadow — slight darkening at the body's silhouette (depth)
    {
        const float rimR = bodyR - 0.4f;
        g.setColour(rimShadow);
        g.drawEllipse(cx - rimR, cy - rimR, rimR * 2.0f, rimR * 2.0f, 0.9f);
    }

    // 5. Specular highlight — the bright catchlight on the upper-left
    {
        const float specCX = cx - bodyR * 0.30f;
        const float specCY = cy - bodyR * 0.42f;
        const float specR  = bodyR * 0.55f;
        juce::ColourGradient spec(juce::Colours::white.withAlpha(0.22f), specCX, specCY,
                                   juce::Colours::transparentWhite,      specCX + specR, specCY + specR,
                                   true);
        g.setGradientFill(spec);
        g.fillEllipse(specCX - specR, specCY - specR, specR * 2.0f, specR * 2.0f);
    }

    // 6. Centre cap — a tiny darker disc at dead-centre for "machined" feel
    {
        const float capR = bodyR * 0.16f;
        juce::ColourGradient cap(bodyBot.brighter(0.20f), cx - capR * 0.3f, cy - capR * 0.3f,
                                  bodyBot.darker(0.30f),  cx + capR,        cy + capR,
                                  true);
        g.setGradientFill(cap);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f, 0.5f);
    }

    // 7. Value arc — luminous ring just outside the bezel
    {
        const float arcR = radius + 2.5f;
        juce::Path arcBg;
        arcBg.addCentredArc(cx, cy, arcR, arcR, 0.0f,
                            rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(arcCol.withAlpha(0.16f));
        g.strokePath(arcBg, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));

        if (sliderPosProportional > 0.01f)
        {
            juce::Path arcVal;
            arcVal.addCentredArc(cx, cy, arcR, arcR, 0.0f,
                                 rotaryStartAngle, toAngle, true);
            // Wide glow
            g.setColour(arcCol.withAlpha(0.32f));
            g.strokePath(arcVal, juce::PathStrokeType(5.5f, juce::PathStrokeType::curved));
            // Main arc
            g.setColour(arcCol.withAlpha(0.88f));
            g.strokePath(arcVal, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved));
        }
    }

    // 8. Tick marks at min / centre / max (sérigraphie etched look)
    {
        const float tickR   = radius + 5.5f;
        const float tickLen = 2.5f;
        g.setColour(currentScheme.text.withAlpha(0.22f));
        auto drawTick = [&](float a)
        {
            const float s = std::sin(a), c = std::cos(a);
            g.drawLine(cx + (tickR - tickLen) * s, cy - (tickR - tickLen) * c,
                       cx + tickR * s,             cy - tickR * c, 0.9f);
        };
        drawTick(rotaryStartAngle);
        drawTick(rotaryEndAngle);
        drawTick((rotaryStartAngle + rotaryEndAngle) * 0.5f);
    }

    // 9. Indicator — drawn AFTER the body so it sits on top
    {
        const float indLen   = bodyR - 2.5f;
        const float indStart = bodyR * 0.22f;
        const float s = std::sin(toAngle), c = std::cos(toAngle);
        const float tipX  = cx + indLen   * s, tipY  = cy - indLen   * c;
        const float baseX = cx + indStart * s, baseY = cy - indStart * c;

        // Subtle cast shadow (offset down-right)
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.drawLine(baseX + 0.6f, baseY + 0.8f, tipX + 0.6f, tipY + 0.8f, 3.2f);
        // Outer glow
        g.setColour(indCol.withAlpha(0.35f));
        g.drawLine(baseX, baseY, tipX, tipY, 5.5f);
        // Main indicator
        g.setColour(indCol);
        g.drawLine(baseX, baseY, tipX, tipY, 3.0f);
        // White pinstripe in the centre — gives 3D relief
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.drawLine(baseX, baseY, tipX, tipY, 1.0f);
    }

    // 10. Value readout — centred in the knob (Serum/Vital style)
    if (slider.getComponentID() != "masterKnob")
    {
        juce::String valueText;
        const double val = slider.getValue();
        if (val == (int)val && std::abs(val) < 1000)        valueText = juce::String((int)val);
        else if (std::abs(val) >= 100)                       valueText = juce::String(val, 0);
        else if (std::abs(val) >= 10)                        valueText = juce::String(val, 1);
        else                                                 valueText = juce::String(val, 2);

        float fontSize = juce::jmax(7.0f, bodyR * 0.55f);
        fontSize = juce::jmin(fontSize, 11.0f);
        g.setFont(juce::Font(juce::FontOptions("JetBrains Mono", fontSize, juce::Font::bold)));

        const float textAlpha = slider.isMouseOverOrDragging() ? 0.95f : 0.78f;

        // 1px text shadow for legibility on the gradient body
        g.setColour(juce::Colours::black.withAlpha(textAlpha * 0.55f));
        g.drawText(valueText, (int)(cx - bodyR + 1.0f), (int)(cy - fontSize / 2.0f + 1.0f),
                   (int)(bodyR * 2.0f), (int)(fontSize + 2.0f),
                   juce::Justification::centred, false);

        g.setColour(centreText.withAlpha(textAlpha));
        g.drawText(valueText, (int)(cx - bodyR), (int)(cy - fontSize / 2.0f),
                   (int)(bodyR * 2.0f), (int)(fontSize + 2.0f),
                   juce::Justification::centred, false);
    }
}

void TsyganatorLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                                             const juce::Slider::SliderStyle, juce::Slider& slider)
{
    // Procedural vertical fader: recessed groove, LED segments, 3D thumb.
    // Theme-aware via currentScheme — reads well in both modes.
    float centerX = (float)x + width / 2.0f;
    float topY = (float)y + 6.0f;
    float bottomY = (float)(y + height) - 6.0f;

    // Clamp thumb position so it never overlaps panel borders
    sliderPos = juce::jlimit(topY + 5.0f, bottomY - 5.0f, sliderPos);

    // === Tick marks on sides (subtle scale) ===
    {
        g.setColour(currentScheme.text.withAlpha(0.15f));
        float tickSpacing = (bottomY - topY) / 10.0f;
        for (int i = 0; i <= 10; ++i)
        {
            float ty = topY + i * tickSpacing;
            g.drawLine(centerX - 8.0f, ty, centerX - 4.0f, ty, 0.5f);
            g.drawLine(centerX + 4.0f, ty, centerX + 8.0f, ty, 0.5f);
        }
    }

    // === Recessed groove (narrow center channel) ===
    {
        float grooveW = 5.0f;
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(centerX - grooveW / 2.0f, topY, grooveW, bottomY - topY, 2.5f);
        // Inset shadow
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.drawLine(centerX - grooveW / 2.0f, topY + 2.0f, centerX - grooveW / 2.0f, bottomY - 2.0f, 0.5f);
        // Highlight on right side
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.drawLine(centerX + grooveW / 2.0f, topY + 2.0f, centerX + grooveW / 2.0f, bottomY - 2.0f, 0.5f);
    }

    // === LED fill segments (bars from bottom up to thumb) ===
    {
        float segW = 10.0f;
        float segH = 2.5f;
        float segGap = 1.0f;
        float segX = centerX - segW / 2.0f;

        g.setColour(currentScheme.accent.withAlpha(0.7f));
        for (float sy = bottomY - 2.0f; sy > sliderPos; sy -= (segH + segGap))
        {
            g.fillRoundedRectangle(segX, sy - segH, segW, segH, 1.0f);
        }
        // Glow behind segments
        if (sliderPos < bottomY - 4.0f)
        {
            float fillH = bottomY - sliderPos;
            g.setColour(currentScheme.accent.withAlpha(0.08f));
            g.fillRect(segX - 2.0f, sliderPos, segW + 4.0f, fillH);
        }
    }

    // === 3D Thumb bar ===
    {
        float thumbW = 18.0f;
        float thumbH = 8.0f;
        float thumbX = centerX - thumbW / 2.0f;
        float thumbY = sliderPos - thumbH / 2.0f;

        // Shadow under thumb
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillRoundedRectangle(thumbX + 1.0f, thumbY + 2.0f, thumbW, thumbH, 2.0f);

        // Thumb body gradient
        juce::ColourGradient thumbGrad(currentScheme.knobOutline.brighter(0.2f), thumbX, thumbY,
                                        currentScheme.knobOutline.darker(0.15f), thumbX, thumbY + thumbH, false);
        g.setGradientFill(thumbGrad);
        g.fillRoundedRectangle(thumbX, thumbY, thumbW, thumbH, 2.0f);

        // Top highlight
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.drawLine(thumbX + 2.0f, thumbY + 1.0f, thumbX + thumbW - 2.0f, thumbY + 1.0f, 1.0f);

        // Center groove line
        g.setColour(currentScheme.accent.withAlpha(0.5f));
        g.drawLine(thumbX + 3.0f, thumbY + thumbH / 2.0f, thumbX + thumbW - 3.0f, thumbY + thumbH / 2.0f, 1.0f);

        // Bottom shadow
        g.setColour(juce::Colours::black.withAlpha(0.1f));
        g.drawLine(thumbX + 2.0f, thumbY + thumbH - 1.0f, thumbX + thumbW - 2.0f, thumbY + thumbH - 1.0f, 0.5f);
    }

    // === Value readout — centered pill on hover/drag only (no overflow risk) ===
    if (slider.isMouseOverOrDragging())
    {
        juce::String valueText;
        double val = slider.getValue();
        if (val == (int)val && std::abs(val) < 1000)
            valueText = juce::String((int)val);
        else if (std::abs(val) >= 10)
            valueText = juce::String(val, 1);
        else
            valueText = juce::String(val, 2);

        g.setFont(juce::Font(juce::FontOptions("JetBrains Mono", 9.0f, juce::Font::bold)));

        float pillW = (float)width - 4.0f;
        float pillH = 14.0f;
        float pillX = centerX - pillW / 2.0f;
        float pillY = sliderPos - pillH - 6.0f;
        pillY = juce::jmax(pillY, topY);

        g.setColour(currentScheme.background.withAlpha(0.85f));
        g.fillRoundedRectangle(pillX, pillY, pillW, pillH, 3.0f);
        g.setColour(currentScheme.accent.withAlpha(0.5f));
        g.drawRoundedRectangle(pillX, pillY, pillW, pillH, 3.0f, 0.8f);
        g.setColour(currentScheme.primary.withAlpha(0.95f));
        g.drawText(valueText, (int)pillX, (int)pillY, (int)pillW, (int)pillH,
                   juce::Justification::centred, false);
    }
}


void TsyganatorLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds();

    // Sequencer step buttons: fully transparent — paint() handles all visuals
    if (button.getComponentID() == "seqStep")
        return;

    // TSYGANIZE button: Belgian = navy bg + yellow text; Italian = transparent bg + pink border
    if (button.getComponentID() == "tsyganize")
    {
        bool isBelgian = (currentScheme.primary == belgianScheme.primary);
        if (isBelgian)
        {
            auto bg = currentScheme.background;
            if (isMouseOverButton) bg = bg.brighter(0.15f);
            if (isButtonDown) bg = bg.darker(0.1f);
            g.setColour(bg);
            g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
        }
        else
        {
            // Italian: transparent background (just border)
            if (isMouseOverButton)
            {
                g.setColour(currentScheme.accent.withAlpha(0.1f));
                g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
            }
        }
        g.setColour(currentScheme.accent);
        g.drawRoundedRectangle(bounds.toFloat(), 4.0f, 2.0f);
        return;
    }

    // Mode switch button: pill shape — text now centred edge-to-edge.
    // P43 D1: removed the LED dot that lived inside the pill (it was
    // redundant with the external MIDI LED at x=1078 next to the button).
    if (button.getComponentID() == "modeSwitch")
    {
        bool isBelgian = (currentScheme.primary == belgianScheme.primary);
        auto boundsF = bounds.toFloat().reduced(1.0f);
        float cornerR = boundsF.getHeight() / 2.0f;  // full stadium/pill shape

        // Fill
        auto modeBg = isBelgian ? juce::Colour(0xFFEFD03A) : juce::Colour(0xFF120822);
        if (isMouseOverButton) modeBg = modeBg.brighter(0.08f);
        if (isButtonDown) modeBg = modeBg.darker(0.05f);
        g.setColour(modeBg);
        g.fillRoundedRectangle(boundsF, cornerR);

        // Thick accent border (3px)
        g.setColour(currentScheme.accent);
        g.drawRoundedRectangle(boundsF, cornerR, 3.0f);

        return;
    }

    // Chorus toggle switches: tall rectangle with dot indicator (matches HTML .toggle-switch)
    if (button.getComponentID() == "chorusToggle")
    {
        bool isOn = button.getToggleState();
        auto boundsF = bounds.toFloat();

        if (isOn)
        {
            // Active: accent color fill with glow
            g.setColour(currentScheme.accent);
            g.fillRoundedRectangle(boundsF, 3.0f);
            // Glow
            g.setColour(currentScheme.accent.withAlpha(0.25f));
            g.fillRoundedRectangle(boundsF.expanded(2.0f), 4.0f);
            // Bottom inner shadow
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.fillRect(boundsF.getX() + 1, boundsF.getBottom() - 4.0f, boundsF.getWidth() - 2, 4.0f);
        }
        else
        {
            // Inactive: recessed look
            g.setColour(currentScheme.buttonFill);
            g.fillRoundedRectangle(boundsF, 3.0f);
            // Inset shadow
            g.setColour(juce::Colours::black.withAlpha(0.12f));
            g.fillRect(boundsF.getX() + 1, boundsF.getY() + 1, boundsF.getWidth() - 2, 3.0f);
            // Border
            g.setColour(juce::Colours::black.withAlpha(0.1f));
            g.drawRoundedRectangle(boundsF, 3.0f, 1.0f);
        }

        // Dot indicator at top center
        float dotSize = 6.0f;
        float dotX = boundsF.getCentreX() - dotSize / 2.0f;
        float dotY = boundsF.getY() + 10.0f;
        if (isOn)
        {
            g.setColour(juce::Colours::white.withAlpha(0.7f));
        }
        else
        {
            g.setColour(juce::Colours::black.withAlpha(0.1f));
        }
        g.fillEllipse(dotX, dotY, dotSize, dotSize);
        return;
    }

    // Generic buttons — P27 harmonised with knob depth:
    // a soft top→bottom gradient and a subtle inner shadow at the bottom
    // give them the same 3D feel as the knobs, so the whole interface
    // shares one visual language.
    auto drawHarmonisedButton = [&](juce::Colour baseFill, bool isOn)
    {
        auto fillCol = baseFill;
        if (isMouseOverButton) fillCol = fillCol.brighter(0.10f);
        if (isButtonDown)      fillCol = fillCol.darker(0.05f);

        const auto top = fillCol.brighter(0.08f);
        const auto bot = fillCol.darker(0.10f);
        const auto rect = bounds.toFloat();

        // Soft vertical gradient — like a slightly bulging button surface
        juce::ColourGradient grad(top, rect.getX(), rect.getY(),
                                   bot, rect.getX(), rect.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRoundedRectangle(rect, 3.0f);

        // Crisp top highlight (catches light)
        g.setColour(juce::Colours::white.withAlpha(isButtonDown ? 0.04f : 0.12f));
        g.drawLine(rect.getX() + 3.0f, rect.getY() + 1.0f,
                   rect.getRight() - 3.0f, rect.getY() + 1.0f, 0.6f);
        // Bottom inner shadow (gives depth)
        g.setColour(juce::Colours::black.withAlpha(0.16f));
        g.drawLine(rect.getX() + 3.0f, rect.getBottom() - 1.5f,
                   rect.getRight() - 3.0f, rect.getBottom() - 1.5f, 0.6f);

        // Outline
        g.setColour(currentScheme.text.withAlpha(isOn ? 0.45f : 0.55f));
        g.drawRoundedRectangle(rect, 3.0f, 1.0f);

        // Active state: external glow + brighter inner ring
        if (isOn)
        {
            g.setColour(currentScheme.accent.withAlpha(0.18f));
            g.fillRoundedRectangle(rect.expanded(2.0f), 4.0f);
            g.setColour(currentScheme.accent.withAlpha(0.20f));
            g.fillRoundedRectangle(rect.reduced(1.0f), 2.5f);
        }
    };

    if (button.getClickingTogglesState())
    {
        const auto baseFill = button.getToggleState() ? currentScheme.accent
                                                       : currentScheme.buttonFill;
        drawHarmonisedButton(baseFill, button.getToggleState());
    }
    else
    {
        drawHarmonisedButton(currentScheme.buttonFill, false);
    }
}

void TsyganatorLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                          bool isMouseOverButton, bool isButtonDown)
{
    // Sequencer step buttons: no text — paint() draws note names, step numbers etc.
    if (button.getComponentID() == "seqStep")
        return;

    // INIT button: accent-colored text
    if (button.getComponentID() == "initBtn")
    {
        g.setFont(juce::Font(juce::FontOptions("Outfit", 10.0f, juce::Font::bold)));
        g.setColour(currentScheme.accent);
        g.drawFittedText(button.getButtonText(), button.getLocalBounds(),
                        juce::Justification::centred, 1);
        return;
    }

    // Mode switch: uppercase accent text, BIGGER + edge-to-edge centred.
    // P43 D3: 13pt → 17pt + balanced 16 px inset each side. The text now
    // dominates the pill comfortably and reads from across the room.
    if (button.getComponentID() == "modeSwitch")
    {
        g.setFont(juce::Font(juce::FontOptions("Outfit", 17.0f, juce::Font::bold))
                      .withExtraKerningFactor(0.04f));
        g.setColour(currentScheme.accent);
        auto textArea = button.getLocalBounds().reduced(16, 0);
        g.drawFittedText(button.getButtonText().toUpperCase(), textArea,
                        juce::Justification::centred, 1);
        return;
    }

    // Chorus toggle switches: label drawn at the bottom of the toggle
    if (button.getComponentID() == "chorusToggle")
    {
        auto area = button.getLocalBounds();
        g.setFont(juce::Font(juce::FontOptions("Outfit", 9.0f, juce::Font::bold)));
        bool isOn = button.getToggleState();
        g.setColour(isOn ? juce::Colours::white.withAlpha(0.95f) : currentScheme.text.withAlpha(0.7f));
        g.drawFittedText(button.getButtonText(),
                        area.removeFromBottom(14),
                        juce::Justification::centred, 1);
        return;
    }

    // TSYGANIZE button: yellow/light text on dark background — large, bold.
    // P43 D4: 16pt → 19pt + slight tracking. Text reads as the main CTA.
    if (button.getComponentID() == "tsyganize")
    {
        g.setFont(juce::Font(juce::FontOptions("Outfit", 19.0f, juce::Font::bold))
                      .withExtraKerningFactor(0.03f));
        g.setColour(currentScheme.primary);
        g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(10, 0),
                        juce::Justification::centred, 1);
        return;
    }

    // Generic buttons — no forced uppercase
    g.setFont(juce::Font(juce::FontOptions("Outfit", 12.0f, juce::Font::bold)));

    // When toggle is ON, background is accent (blue/pink) — use high-contrast text
    if (button.getClickingTogglesState() && button.getToggleState())
    {
        bool isBelgian = (currentScheme.primary == belgianScheme.primary);
        g.setColour(isBelgian ? currentScheme.primary : juce::Colours::white);
    }
    else
        g.setColour(currentScheme.text);

    g.drawFittedText(button.getButtonText(), button.getLocalBounds(),
                    juce::Justification::centred, 1);
}

void TsyganatorLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height,
                                         bool isButtonDown, int buttonX, int buttonY,
                                         int buttonW, int buttonH, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height);
    g.setColour(currentScheme.background);   // navy/dark purple bg (matches popup)
    g.fillRoundedRectangle(bounds.toFloat(), 3.0f);

    g.setColour(currentScheme.accent.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.toFloat(), 3.0f, 1.0f);

    // Draw dropdown arrow
    g.setColour(currentScheme.primary);  // yellow/pink arrow
    auto arrowX = (float)(width - 15);
    auto arrowY = (float)(height / 2 - 3);
    g.drawLine(arrowX, arrowY, arrowX + 4.0f, arrowY + 4.0f, 2.0f);
    g.drawLine(arrowX + 4.0f, arrowY + 4.0f, arrowX + 8.0f, arrowY, 2.0f);
}

void TsyganatorLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                             bool isSeparator, bool isActive, bool isHighlighted,
                                             bool isTicked, bool hasSubMenu, const juce::String& text,
                                             const juce::String& shortcutKeyText, const juce::Drawable* icon,
                                             const juce::Colour* textColour)
{
    if (isSeparator)
    {
        g.setColour(currentScheme.text.withAlpha(0.2f));
        g.drawLine((float)area.getX() + 4, (float)area.getCentreY(),
                   (float)area.getRight() - 4, (float)area.getCentreY(), 1.0f);
        return;
    }

    // Section headings (from addSectionHeading) — bold, accent-colored, non-interactive
    if (!isActive && !isSeparator)
    {
        g.setColour(currentScheme.accent.withAlpha(0.25f));
        g.fillRect(area);
        g.setColour(currentScheme.primary);  // yellow (Belgian) / light pink (Italian)
        g.setFont(juce::Font(juce::FontOptions("Outfit", 10.0f, juce::Font::bold)));
        g.drawText(text, area.reduced(8, 0), juce::Justification::centredLeft);
        return;
    }

    // Regular items: dark bg with bright text
    if (isHighlighted)
    {
        g.setColour(currentScheme.accent);
        g.fillRect(area);
        g.setColour(juce::Colours::white);
    }
    else
    {
        g.setColour(currentScheme.background);
        g.fillRect(area);
        g.setColour(currentScheme.primary);  // yellow (Belgian) / light pink (Italian)
    }

    g.setFont(juce::Font(juce::FontOptions("Outfit", 11.0f, juce::Font::plain)));
    g.drawText(text, area.reduced(10, 0), juce::Justification::centredLeft);
}

void TsyganatorLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                                       int standardMenuItemHeight, int& idealWidth,
                                                       int& idealHeight)
{
    if (isSeparator)
    {
        idealWidth = 50;
        idealHeight = 2;
    }
    else
    {
        idealWidth = juce::jmax(100, (int)(text.length() * 7));
        idealHeight = 22;  // compact items — prevents giant popup
    }
}

void TsyganatorLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    // Belgian: navy blue bg; Italian: dark purple bg
    g.fillAll(currentScheme.background);
    g.setColour(currentScheme.accent.withAlpha(0.4f));
    g.drawRect(0, 0, width, height, 1);
}

juce::Typeface::Ptr TsyganatorLookAndFeel::getTypefaceForFont(const juce::Font& font)
{
    auto name = font.getTypefaceName();

    if (name.containsIgnoreCase("Outfit"))
    {
        // ExtraBold for heavier weights, Bold for everything else
        if (font.isBold() && name.containsIgnoreCase("ExtraBold"))
            return outfitExtraBoldTypeface;
        return outfitBoldTypeface;
    }

    if (name.containsIgnoreCase("JetBrains"))
        return jetBrainsMonoBoldTypeface;

    // Fall back to default system font for anything else
    return LookAndFeel_V4::getTypefaceForFont(font);
}

//==============================================================================
// TsyganatorEditor Implementation
//==============================================================================

// Helper to get MIDI note name
static juce::String midiNoteName(int note)
{
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int oct = (note / 12) - 1;
    return juce::String(names[note % 12]) + juce::String(oct);
}

TsyganatorEditor::TsyganatorEditor(TsyganatorProcessor& p)
    : juce::AudioProcessorEditor(&p), processor(p)
{
    setSize(1360, 600);
    setOpaque(true);  // Tells host this component is fully opaque (no transparent pixels).

    // P36c v7 — REVERTED OpenGL (v6). It improved the wedge slightly
    // but introduced text-rendering corruption on macOS Tahoe (glyph
    // spacing broken on partial renders). We're back to software
    // rendering — the next step is updating JUCE itself (8.0.4 → 8.0.10)
    // which contains several VST3 / paint fixes accumulated over 6
    // point releases.

    // Force JUCE to call paint() with the FULL component bounds as
    // the clip region (skips clip-region optimisation).
    setPaintingIsUnclipped(true);

    // Disable buffered-to-image — paint goes straight to screen.
    setBufferedToImage(false);

    setLookAndFeel(&lookAndFeel);

    // ========== Load Images from BinaryData ==========
    // Cleanup L: removed loads for bg_*, knob_filmstrip_*, fader_*, led_segment_*,
    // mascot_smiley/disco — those PNGs were never drawn (UI is fully procedural).

    // LED diodes (on/off states) — drawn in the step sequencer
    ledOnBelgian = juce::ImageCache::getFromMemory(BinaryData::led_on_belgian_png, BinaryData::led_on_belgian_pngSize);
    ledOffBelgian = juce::ImageCache::getFromMemory(BinaryData::led_off_belgian_png, BinaryData::led_off_belgian_pngSize);
    ledOnItalian = juce::ImageCache::getFromMemory(BinaryData::led_on_italian_png, BinaryData::led_on_italian_pngSize);
    ledOffItalian = juce::ImageCache::getFromMemory(BinaryData::led_off_italian_png, BinaryData::led_off_italian_pngSize);

    // Parse reference SVG for pixel-perfect acid smiley (Noun Project icon)
    {
        auto svgXml = juce::XmlDocument::parse(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 59.13 59.13\">"
            "<path d=\"M29.56,0A29.57,29.57,0,1,0,59.13,29.56,29.57,29.57,0,0,0,29.56,0Z"
            "m7.82,15.06c1.71,0,3.09,2.35,3.09,5.24s-1.38,5.23-3.09,5.23-3.1-2.34-3.1-5.23"
            "S35.66,15.06,37.38,15.06ZM22,15c1.71,0,3.1,2.35,3.1,5.24s-1.39,5.23-3.1,5.23"
            "-3.1-2.34-3.1-5.23S20.3,15,22,15ZM49.75,35.3h0a1.16,1.16,0,0,0-1.11.74l-.12.2"
            "-.15.32a22.56,22.56,0,0,1-1.64,2.85,23.09,23.09,0,0,1-4.46,4.92l-.33.26-.33.25"
            "-.34.26-.34.24-.35.24-.36.23-.36.22-.36.22a21.57,21.57,0,0,1-3.07,1.46A19.77,"
            "19.77,0,0,1,29.74,49a19.55,19.55,0,0,1-3.42-.25A20,20,0,0,1,23,47.89,20.63,"
            "20.63,0,0,1,19.9,46.5c-.24-.14-.49-.27-.73-.42s-.48-.29-.71-.45-.47-.31-.7-.48"
            "l-.67-.5a22.72,22.72,0,0,1-4.57-4.83A22.38,22.38,0,0,1,10.83,37l-.18-.35s-.07"
            "-.11-.11-.19a1,1,0,0,0-1-.63l-.16,0h0a.75.75,0,0,1-.74-.26c-.26-.4.12-1.08.83"
            "-1.53s1.48-.5,1.73-.1a.77.77,0,0,1-.09.81h0v0l0,0a1.11,1.11,0,0,0-.2,1l.57.76"
            "A29.86,29.86,0,0,0,13.49,39a26.59,26.59,0,0,0,4.77,4c.21.14.43.27.65.4s.44.26"
            ".66.38l.68.37.68.33a21.17,21.17,0,0,0,2.83,1.09,18.82,18.82,0,0,0,2.94.64,17.79,"
            "17.79,0,0,0,3,.19,18.79,18.79,0,0,0,3-.26,19.79,19.79,0,0,0,2.92-.73,21.26,"
            "21.26,0,0,0,2.81-1.15l.34-.17.33-.19.34-.18.33-.19.33-.2.32-.2.33-.21.32-.21"
            "a26.62,26.62,0,0,0,4.68-4.09,29.6,29.6,0,0,0,2-2.44c.18-.24.36-.49.53-.74"
            "a.85.85,0,0,0-.17-1h0a.85.85,0,0,1-.23-.94c.23-.41,1-.4,1.74,0s1.13,1.09.89,"
            "1.5A.79.79,0,0,1,49.75,35.3Z\"/></svg>"
        );
        if (svgXml)
            smileyDrawable = juce::Drawable::createFromSVG(*svgXml);
    }

    // Logo PNGs (WordArt-style gradient logos)
    logoBelgian = juce::ImageCache::getFromMemory(BinaryData::logo_belgian_png, BinaryData::logo_belgian_pngSize);
    logoItalian = juce::ImageCache::getFromMemory(BinaryData::logo_italian_png, BinaryData::logo_italian_pngSize);

    // P33: load the user-supplied acid smiley SVG once. Drawn into a
    // square box centred on (mascotCX, mascotCY) at faceR size during
    // paint(). Falls back gracefully (procedural smiley) if loading fails.
    acidSmileyDrawable = juce::Drawable::createFromImageData(
        BinaryData::acid_smiley_svg, BinaryData::acid_smiley_svgSize);

    // Cleanup L: lookAndFeel.setImages(...) call removed — the LookAndFeel
    // no longer stores image assets (UI is fully procedural).

    // ========== OSC1 Sliders ==========
    // Label styling helper — small, themed color, no forced caps
    auto styleLabel = [](juce::Label& label) {
        label.setFont(juce::Font(juce::FontOptions("Outfit", 10.0f, juce::Font::bold)));
        label.setColour(juce::Label::textColourId, juce::Colour(0xFFE8B0C8));  // visible default (Italian); syncMode adjusts for Belgian
    };

    auto setupFader = [this, &styleLabel](juce::Slider& slider, juce::Label& label,
                             std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach,
                             const juce::String& paramId, const juce::String& labelText)
    {
        slider.setSliderStyle(juce::Slider::LinearBarVertical);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setPaintingIsUnclipped(true);
        slider.setRepaintsOnMouseActivity(true);
        addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, paramId, slider);
        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        styleLabel(label);
        addAndMakeVisible(label);
    };

    auto setupKnob = [this, &styleLabel](juce::Slider& slider, juce::Label& label,
                            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attach,
                            const juce::String& paramId, const juce::String& labelText)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setPaintingIsUnclipped(true);
        slider.setRepaintsOnMouseActivity(true);  // Repaint on hover for tooltip
        addAndMakeVisible(slider);
        attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.apvts, paramId, slider);
        label.setText(labelText, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        styleLabel(label);
        addAndMakeVisible(label);
    };

    // OSC1
    setupFader(sawLevelSlider, sawLevelLabel, sawLevelAttach, "sawLevel", "Saw");
    setupFader(pulseLevelSlider, pulseLevelLabel, pulseLevelAttach, "pulseLevel", "Pulse");
    setupFader(triangleLevelSlider, triangleLevelLabel, triangleLevelAttach, "triangleLevel", "Tri");
    setupFader(subLevelSlider, subLevelLabel, subLevelAttach, "subLevel", "Sub");
    setupFader(noiseLevelSlider, noiseLevelLabel, noiseLevelAttach, "noiseLevel", "Noise");
    setupFader(pulseWidthSlider, pulseWidthLabel, pulseWidthAttach, "pulseWidth", "Width");
    setupFader(osc1VolumeSlider, osc1VolumeLabel, osc1VolumeAttach, "osc1Volume", "Level");

    osc1Label.setText("Osc 1", juce::dontSendNotification);
    osc1Label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osc1Label);

    // ========== OSC2 Sliders ==========
    setupFader(osc2SawSlider, osc2SawLabel, osc2SawAttach, "osc2Saw", "Saw");
    setupFader(osc2PulseSlider, osc2PulseLabel, osc2PulseAttach, "osc2Pulse", "Pulse");
    setupFader(osc2TriangleSlider, osc2TriangleLabel, osc2TriangleAttach, "osc2Triangle", "Tri");
    setupFader(osc2PWSlider, osc2PWLabel, osc2PWAttach, "osc2PW", "Width");
    setupFader(osc2FineSlider, osc2FineLabel, osc2FineAttach, "osc2Fine", "Fine");
    setupFader(osc2VolumeSlider, osc2VolumeLabel, osc2VolumeAttach, "osc2Volume", "Level");

    // osc2Octave: parameter has 5 choices: "-2", "-1", "0", "+1", "+2"
    osc2OctaveCombo.addItem("-2", 1);
    osc2OctaveCombo.addItem("-1", 2);
    osc2OctaveCombo.addItem("0", 3);
    osc2OctaveCombo.addItem("+1", 4);
    osc2OctaveCombo.addItem("+2", 5);
    addAndMakeVisible(osc2OctaveCombo);
    osc2OctaveAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "osc2Octave", osc2OctaveCombo);
    osc2OctaveLabel.setText("Octave", juce::dontSendNotification);
    osc2OctaveLabel.setJustificationType(juce::Justification::centred);
    styleLabel(osc2OctaveLabel);
    addAndMakeVisible(osc2OctaveLabel);

    osc2Label.setText("Osc 2", juce::dontSendNotification);
    osc2Label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(osc2Label);

    // ========== Filter Section ==========
    setupKnob(cutoffSlider, cutoffLabel, cutoffAttach, "cutoff", "Cutoff");
    setupKnob(resonanceSlider, resonanceLabel, resonanceAttach, "resonance", "Reso");
    setupKnob(filterEnvAmountSlider, filterEnvAmountLabel, filterEnvAmountAttach, "filterEnvAmount", "Env Amt");

    filterLabel.setText("Filter", juce::dontSendNotification);
    filterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(filterLabel);

    // ========== Filter ADSR ==========
    setupFader(filterAttackSlider, filterAttackLabel, filterAttackAttach, "filterAttack", "Attack");
    setupFader(filterDecaySlider, filterDecayLabel, filterDecayAttach, "filterDecay", "Decay");
    setupFader(filterSustainSlider, filterSustainLabel, filterSustainAttach, "filterSustain", "Sustain");
    setupFader(filterReleaseSlider, filterReleaseLabel, filterReleaseAttach, "filterRelease", "Release");

    filterADSRLabel.setText("Filter ADSR", juce::dontSendNotification);
    filterADSRLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(filterADSRLabel);

    // ========== Amp ADSR ==========
    setupFader(ampAttackSlider, ampAttackLabel, ampAttackAttach, "ampAttack", "Attack");
    setupFader(ampDecaySlider, ampDecayLabel, ampDecayAttach, "ampDecay", "Decay");
    setupFader(ampSustainSlider, ampSustainLabel, ampSustainAttach, "ampSustain", "Sustain");
    setupFader(ampReleaseSlider, ampReleaseLabel, ampReleaseAttach, "ampRelease", "Release");

    ampADSRLabel.setText("Amp ADSR", juce::dontSendNotification);
    ampADSRLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(ampADSRLabel);

    // ========== Performance Section ==========
    // unisonMode: parameter has 2 choices: "Off", "On"
    // Hidden ComboBox keeps APVTS attachment alive
    unisonModeCombo.addItem("Off", 1);
    unisonModeCombo.addItem("On", 2);
    addAndMakeVisible(unisonModeCombo);
    unisonModeCombo.setBounds(-100, -100, 1, 1);  // hidden
    unisonModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "unisonMode", unisonModeCombo);

    // Visible toggle button for UNISON
    unisonButton.setButtonText("Unison");
    unisonButton.setClickingTogglesState(true);
    unisonButton.onClick = [this]() {
        auto* param = processor.apvts.getParameter("unisonMode");
        float newVal = unisonButton.getToggleState() ? 1.0f : 0.0f;
        param->setValueNotifyingHost(param->convertTo0to1(newVal));
    };
    addAndMakeVisible(unisonButton);

    unisonModeLabel.setBounds(-100, -100, 1, 1);  // hidden (label baked in BG)

    setupKnob(unisonDetuneSlider, unisonDetuneLabel, unisonDetuneAttach, "unisonDetune", "Detune");
    setupKnob(keyTrackingSlider, keyTrackingLabel, keyTrackingAttach, "keyTracking", "Key Trk");
    setupKnob(portamentoSlider, portamentoLabel, portamentoAttach, "portamento", "Glide");

    performanceLabel.setText("Performance", juce::dontSendNotification);
    performanceLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(performanceLabel);

    // ========== Effects Section ==========
    setupKnob(globalFineTuneSlider, globalFineTuneLabel, globalFineTuneAttach, "globalFineTune", "Tune");

    // P37: Crush button + Crush amount knob + their LoFi APVTS attachment have
    // been REMOVED. The LoFi DSP module is gone — the EFFECTS card now hosts
    // just Sat + Chorus (P39 merge).

    // Visible toggle button for the saturation/colouring stage.
    // P30 rebrand: "Sat" (was "Master") lifts ambiguity with the MASTER
    // section on the far right of Row 2. P37/P39: Crush removed; Sat is
    // now the sole non-modulating colouring stage in the EFFECTS card,
    // paired with Chorus (4 toggles) on the right.
    vintageButton.setButtonText("Vintage");
    vintageButton.setClickingTogglesState(true);
    vintageButton.onClick = [this]() {
        auto* param = processor.apvts.getParameter("vintageMode");
        float newVal = vintageButton.getToggleState() ? 1.0f : 0.0f;
        param->setValueNotifyingHost(param->convertTo0to1(newVal));
    };
    addAndMakeVisible(vintageButton);

    // P30: rebrand label "Master Amt" → "Sat Amt" for the same reason.
    setupKnob(vintageAmountSlider, vintageAmountLabel, vintageAmountAttach, "vintageAmount", "Amount");

    effectsLabel.setText("Effects", juce::dontSendNotification);
    effectsLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(effectsLabel);

    // ========== LFO Section ==========
    setupKnob(lfoRateSlider, lfoRateLabel, lfoRateAttach, "lfoRate", "Rate");
    setupKnob(lfoDepthSlider, lfoDepthLabel, lfoDepthAttach, "lfoDepth", "Depth");

    // lfoWaveform: parameter has 5 choices: "Sine", "Triangle", "Saw", "Square", "S&H"
    lfoWaveformCombo.addItem("Sine", 1);
    lfoWaveformCombo.addItem("Tri", 2);
    lfoWaveformCombo.addItem("Saw", 3);
    lfoWaveformCombo.addItem("Sq", 4);
    lfoWaveformCombo.addItem("S&H", 5);
    addAndMakeVisible(lfoWaveformCombo);
    lfoWaveformAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfoWaveform", lfoWaveformCombo);
    lfoWaveformLabel.setText("Wave", juce::dontSendNotification);
    lfoWaveformLabel.setJustificationType(juce::Justification::centred);
    styleLabel(lfoWaveformLabel);
    addAndMakeVisible(lfoWaveformLabel);

    // lfoDestination: parameter has 4 choices: "Cutoff", "Pulse Width", "Pitch", "Volume"
    // P23: shortened "Cutoff" to "Cut" so it no longer truncates to "Cu..."
    // inside the narrow combo box.
    lfoDestinationCombo.addItem("Cut",   1);
    lfoDestinationCombo.addItem("PW",    2);
    lfoDestinationCombo.addItem("Pitch", 3);
    lfoDestinationCombo.addItem("Vol",   4);
    addAndMakeVisible(lfoDestinationCombo);
    lfoDestinationAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfoDestination", lfoDestinationCombo);
    lfoDestinationLabel.setText("Dest", juce::dontSendNotification);
    lfoDestinationLabel.setJustificationType(juce::Justification::centred);
    styleLabel(lfoDestinationLabel);
    addAndMakeVisible(lfoDestinationLabel);

    lfoLabel.setText("LFO", juce::dontSendNotification);  // acronym stays uppercase
    lfoLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lfoLabel);

    // LFO Sync toggle — switches between Free Hz and tempo-synced musical divisions
    lfoSyncButton.setButtonText("Free");
    lfoSyncButton.setClickingTogglesState(true);
    lfoSyncButton.setComponentID("lfoSync");
    lfoSyncButton.onClick = [this]() {
        bool syncOn = lfoSyncButton.getToggleState();
        auto* param = processor.apvts.getParameter("lfoSync");
        param->setValueNotifyingHost(syncOn ? 1.0f : 0.0f);
        lfoSyncButton.setButtonText(syncOn ? "Sync" : "Free");
        // Show/hide Rate knob vs Sync Rate combo
        lfoRateSlider.setVisible(!syncOn);
        lfoRateLabel.setVisible(!syncOn);
        lfoSyncRateCombo.setVisible(syncOn);
        lfoSyncLabel.setVisible(syncOn);
    };
    addAndMakeVisible(lfoSyncButton);

    // LFO Sync Rate combo — musical divisions
    lfoSyncRateCombo.addItem("4/1", 1);
    lfoSyncRateCombo.addItem("2/1", 2);
    lfoSyncRateCombo.addItem("1/1", 3);
    lfoSyncRateCombo.addItem("1/2", 4);
    lfoSyncRateCombo.addItem("1/4", 5);
    lfoSyncRateCombo.addItem("1/8", 6);
    lfoSyncRateCombo.addItem("1/16", 7);
    lfoSyncRateCombo.addItem("1/32", 8);
    lfoSyncRateCombo.addItem("1/4T", 9);
    lfoSyncRateCombo.addItem("1/8T", 10);
    lfoSyncRateCombo.addItem("1/16T", 11);
    lfoSyncRateCombo.addItem("1/2.", 12);
    lfoSyncRateCombo.addItem("1/4.", 13);
    lfoSyncRateCombo.addItem("1/8.", 14);
    addAndMakeVisible(lfoSyncRateCombo);
    lfoSyncRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "lfoSyncRate", lfoSyncRateCombo);
    lfoSyncLabel.setText("Rate", juce::dontSendNotification);
    lfoSyncLabel.setJustificationType(juce::Justification::centred);
    styleLabel(lfoSyncLabel);
    addAndMakeVisible(lfoSyncLabel);

    // Initially: show free rate, hide sync combo (default is Free mode)
    {
        bool syncOn = juce::roundToInt(processor.apvts.getRawParameterValue("lfoSync")->load()) == 1;
        lfoSyncButton.setToggleState(syncOn, juce::dontSendNotification);
        lfoSyncButton.setButtonText(syncOn ? "Sync" : "Free");
        lfoRateSlider.setVisible(!syncOn);
        lfoRateLabel.setVisible(!syncOn);
        lfoSyncRateCombo.setVisible(syncOn);
        lfoSyncLabel.setVisible(syncOn);
    }

    // ========== Chorus Section ==========
    // chorusMode: parameter has 4 choices: "Off", "Chorus I", "Chorus II", "I + II"
    // Hidden ComboBox keeps APVTS attachment alive
    chorusModeCombo.addItem("Off", 1);
    chorusModeCombo.addItem("Chorus I", 2);
    chorusModeCombo.addItem("Chorus II", 3);
    chorusModeCombo.addItem("I + II", 4);
    addAndMakeVisible(chorusModeCombo);
    chorusModeCombo.setBounds(-100, -100, 1, 1);  // hidden — toggle buttons handle UI
    chorusModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "chorusMode", chorusModeCombo);

    // 4 visual toggle switches matching HTML .toggle-switch design
    auto setupChorusToggle = [this](juce::TextButton& btn, int choiceIndex) {
        btn.setClickingTogglesState(true);
        btn.setComponentID("chorusToggle");
        btn.onClick = [this, choiceIndex, &btn]() {
            // Set this toggle ON, all others OFF (mutual exclusion)
            chorusOffBtn.setToggleState(false, juce::dontSendNotification);
            chorusIBtn.setToggleState(false, juce::dontSendNotification);
            chorusIIBtn.setToggleState(false, juce::dontSendNotification);
            chorusIPlusIIBtn.setToggleState(false, juce::dontSendNotification);
            btn.setToggleState(true, juce::dontSendNotification);
            // Set parameter
            auto* param = processor.apvts.getParameter("chorusMode");
            param->setValueNotifyingHost(param->convertTo0to1((float)choiceIndex));
        };
        addAndMakeVisible(btn);
    };
    setupChorusToggle(chorusOffBtn, 0);
    chorusOffBtn.setButtonText("Off");
    setupChorusToggle(chorusIBtn, 1);
    chorusIBtn.setButtonText("I");
    setupChorusToggle(chorusIIBtn, 2);
    chorusIIBtn.setButtonText("II");
    setupChorusToggle(chorusIPlusIIBtn, 3);
    chorusIPlusIIBtn.setButtonText("I+II");
    chorusOffBtn.setToggleState(true, juce::dontSendNotification);  // default OFF

    chorusLabel.setText("Chorus", juce::dontSendNotification);
    chorusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(chorusLabel);

    // ========== Master Section ==========
    setupKnob(masterGainSlider, masterGainLabel, masterGainAttach, "masterGain", "Master");
    masterGainSlider.setComponentID("masterKnob");  // Skip center readout — dB shown below
    addAndMakeVisible(masterDbLabel);
    masterLabel.setText("Master", juce::dontSendNotification);
    masterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterLabel);

    // ========== Sequencer Section ==========
    seqNumStepsSlider.setSliderStyle(juce::Slider::LinearBarVertical);
    seqNumStepsSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    seqNumStepsSlider.setRange(1.0, 16.0, 1.0);
    seqNumStepsSlider.setVisible(false);  // Hidden — we use +/- buttons and numeric label instead
    seqNumStepsAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor.apvts, "seqNumSteps", seqNumStepsSlider);

    setupKnob(seqSwingSlider, seqSwingLabel, seqSwingAttach, "seqSwing", "Swing");
    setupKnob(seqGateLengthSlider, seqGateLengthLabel, seqGateLengthAttach, "seqGateLength", "Gate");

    sequencerLabel.setText("Steps", juce::dontSendNotification);
    sequencerLabel.setJustificationType(juce::Justification::centred);
    styleLabel(sequencerLabel);  // Same font as Swing/Gate labels
    addAndMakeVisible(sequencerLabel);

    // Pattern preset selector — filtered by current mode
    seqPatternCombo.setComponentID("seqPattern");
    seqPatternCombo.addItem("-- Pattern --", 100);  // placeholder
    // Belgian patterns (indices 0..N_BEL-1)
    for (int i = 0; i < StepSequencer::getNumBelgianPatterns(); ++i)
        seqPatternCombo.addItem(StepSequencer::getPattern(i).name, i + 1);
    // Italian patterns (indices N_BEL..N_BEL+N_IT-1)
    for (int i = StepSequencer::getFirstItalianPatternIndex();
         i < StepSequencer::getFirstItalianPatternIndex() + StepSequencer::getNumItalianPatterns(); ++i)
        seqPatternCombo.addItem(StepSequencer::getPattern(i).name, i + 1);
    seqPatternCombo.setSelectedId(100, juce::dontSendNotification);
    seqPatternCombo.onChange = [this]() {
        int sel = seqPatternCombo.getSelectedId();
        if (sel >= 1 && sel <= StepSequencer::NUM_PATTERNS)
        {
            processor.getSequencer().loadPattern(sel - 1);
            // Also set numSteps to 16 when loading a pattern
            auto* param = processor.apvts.getParameter("seqNumSteps");
            if (param) param->setValueNotifyingHost(param->convertTo0to1(16));
            repaint();
        }
    };
    addAndMakeVisible(seqPatternCombo);

    // Sequencer action buttons
    seqRandButton.setButtonText("Rand");
    seqRandButton.onClick = [this]() {
        processor.getSequencer().randomizePattern();
        actionFlash = 1.0f; actionFlashBtn = &seqRandButton;
        repaint();
    };
    addAndMakeVisible(seqRandButton);

    seqClearButton.setButtonText("Clr");
    seqClearButton.onClick = [this]() {
        processor.getSequencer().clearPattern();
        actionFlash = 1.0f; actionFlashBtn = &seqClearButton;
        repaint();
    };
    addAndMakeVisible(seqClearButton);

    seqGlideButton.setButtonText("Glide");
    seqGlideButton.setClickingTogglesState(true);
    seqGlideButton.onClick = [this]() {
        auto& seq = processor.getSequencer();
        int step = selectedStep;
        if (step >= 0 && step < StepSequencer::MAX_STEPS)
        {
            seq.setStepGlide(step, seqGlideButton.getToggleState());
            stepEditFlash = 1.0f; stepEditFlashIdx = step;
            actionFlash = 1.0f; actionFlashBtn = &seqGlideButton;
            repaint();
        }
    };
    addAndMakeVisible(seqGlideButton);

    seqAccentButton.setButtonText("Accent");
    seqAccentButton.setClickingTogglesState(true);
    seqAccentButton.onClick = [this]() {
        auto& seq = processor.getSequencer();
        int step = selectedStep;
        if (step >= 0 && step < StepSequencer::MAX_STEPS)
        {
            seq.setStepAccent(step, seqAccentButton.getToggleState());
            stepEditFlash = 1.0f; stepEditFlashIdx = step;
            actionFlash = 1.0f; actionFlashBtn = &seqAccentButton;
            repaint();
        }
    };
    addAndMakeVisible(seqAccentButton);

    seqNotePlusButton.setButtonText("N+");
    seqNotePlusButton.onClick = [this]() {
        auto& seq = processor.getSequencer();
        int step = selectedStep;
        if (step >= 0 && step < StepSequencer::MAX_STEPS)
        {
            int note = seq.getStep(step).note;
            seq.setStepNote(step, juce::jmin(note + 1, 127));
            stepEditFlash = 1.0f; stepEditFlashIdx = step;
            actionFlash = 1.0f; actionFlashBtn = &seqNotePlusButton;
            repaint();
        }
    };
    addAndMakeVisible(seqNotePlusButton);

    seqNoteMinusButton.setButtonText("N-");
    seqNoteMinusButton.onClick = [this]() {
        auto& seq = processor.getSequencer();
        int step = selectedStep;
        if (step >= 0 && step < StepSequencer::MAX_STEPS)
        {
            int note = seq.getStep(step).note;
            seq.setStepNote(step, juce::jmax(note - 1, 0));
            stepEditFlash = 1.0f; stepEditFlashIdx = step;
            actionFlash = 1.0f; actionFlashBtn = &seqNoteMinusButton;
            repaint();
        }
    };
    addAndMakeVisible(seqNoteMinusButton);

    seqVelPlusButton.setButtonText("V+");
    seqVelPlusButton.onClick = [this]() {
        auto& seq = processor.getSequencer();
        int step = selectedStep;
        if (step >= 0 && step < StepSequencer::MAX_STEPS)
        {
            float vel = seq.getStep(step).velocity;
            seq.setStepVelocity(step, juce::jmin(vel + 0.05f, 1.0f));
            stepEditFlash = 1.0f; stepEditFlashIdx = step;
            actionFlash = 1.0f; actionFlashBtn = &seqVelPlusButton;
            repaint();
        }
    };
    addAndMakeVisible(seqVelPlusButton);

    seqVelMinusButton.setButtonText("V-");
    seqVelMinusButton.onClick = [this]() {
        auto& seq = processor.getSequencer();
        int step = selectedStep;
        if (step >= 0 && step < StepSequencer::MAX_STEPS)
        {
            float vel = seq.getStep(step).velocity;
            seq.setStepVelocity(step, juce::jmax(vel - 0.05f, 0.0f));
            stepEditFlash = 1.0f; stepEditFlashIdx = step;
            actionFlash = 1.0f; actionFlashBtn = &seqVelMinusButton;
            repaint();
        }
    };
    addAndMakeVisible(seqVelMinusButton);

    seqNumStepsLabel.setText("8", juce::dontSendNotification);  // Updated in timerCallback
    seqNumStepsLabel.setJustificationType(juce::Justification::centred);
    seqNumStepsLabel.setFont(juce::Font(juce::FontOptions("JetBrains Mono", 14.0f, juce::Font::bold)));
    styleLabel(seqNumStepsLabel);
    addAndMakeVisible(seqNumStepsLabel);

    stepPlusButton.setButtonText("+");
    stepPlusButton.onClick = [this]() {
        int val = juce::roundToInt(processor.apvts.getRawParameterValue("seqNumSteps")->load());
        if (val < 16)
        {
            auto* param = processor.apvts.getParameter("seqNumSteps");
            param->setValueNotifyingHost(param->convertTo0to1((float)(val + 1)));
            layoutRow4();  // re-layout step buttons for new count
            repaint();
        }
    };
    addAndMakeVisible(stepPlusButton);

    stepMinusButton.setButtonText("-");
    stepMinusButton.onClick = [this]() {
        int val = juce::roundToInt(processor.apvts.getRawParameterValue("seqNumSteps")->load());
        if (val > 1)
        {
            auto* param = processor.apvts.getParameter("seqNumSteps");
            param->setValueNotifyingHost(param->convertTo0to1((float)(val - 1)));
            layoutRow4();  // re-layout step buttons for new count
            repaint();
        }
    };
    addAndMakeVisible(stepMinusButton);

    // Sequencer step buttons (16 max = StepSequencer::MAX_STEPS)
    for (int i = 0; i < maxSequencerSteps; ++i)
    {
        seqStepButtons[i].setButtonText(juce::String(i + 1));
        seqStepButtons[i].setClickingTogglesState(true);
        seqStepButtons[i].setComponentID("seqStep");
        seqStepButtons[i].onClick = [this, i]() {
            selectedStep = i;  // Select this step for editing
            processor.getSequencer().toggleStep(i);
            // Sync Glide/Accent buttons to newly selected step
            const auto& stepData = processor.getSequencer().getStep(i);
            seqGlideButton.setToggleState(stepData.glide, juce::dontSendNotification);
            seqAccentButton.setToggleState(stepData.accent, juce::dontSendNotification);
            repaint();
        };
        addAndMakeVisible(seqStepButtons[i]);
    }

    // ========== Arpeggiator Section ==========
    // arpRate: parameter has 9 choices: "1/1","1/2","1/4","1/8","1/16","1/32","1/4T","1/8T","1/16T"
    arpRateCombo.addItem("1/1", 1);
    arpRateCombo.addItem("1/2", 2);
    arpRateCombo.addItem("1/4", 3);
    arpRateCombo.addItem("1/8", 4);
    arpRateCombo.addItem("1/16", 5);
    arpRateCombo.addItem("1/32", 6);
    arpRateCombo.addItem("1/4T", 7);
    arpRateCombo.addItem("1/8T", 8);
    arpRateCombo.addItem("1/16T", 9);
    addAndMakeVisible(arpRateCombo);
    arpRateAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "arpRate", arpRateCombo);
    arpLabel.setText("Arp Rate", juce::dontSendNotification);
    arpLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(arpLabel);

    // arpMode: 4 choices "Up","Down","Up-Down","Random".
    // The Arpeggiator engine already supports these — we just expose them in the UI.
    arpModeCombo.addItem("Up",      1);
    arpModeCombo.addItem("Down",    2);
    arpModeCombo.addItem("Up-Down", 3);
    arpModeCombo.addItem("Random",  4);
    addAndMakeVisible(arpModeCombo);
    arpModeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.apvts, "arpMode", arpModeCombo);
    arpModeLabel.setText("Mode", juce::dontSendNotification);
    arpModeLabel.setJustificationType(juce::Justification::centred);
    styleLabel(arpModeLabel);
    addAndMakeVisible(arpModeLabel);

    // ========== Mode & Play Mode Buttons ==========
    modeButton.setButtonText("Hard Belgian Mode");
    modeButton.setComponentID("modeSwitch");
    modeButton.onClick = [this]() {
        auto newMode = processor.getMode() == TsyganatorProcessor::BelgianMode
                       ? TsyganatorProcessor::ItalianMode
                       : TsyganatorProcessor::BelgianMode;
        processor.setMode(newMode);
    };
    addAndMakeVisible(modeButton);

    // Play mode buttons — mutual exclusion handled in onClick (instant visual) + timerCallback (sync)
    auto setupPlayModeBtn = [this](juce::TextButton& btn, const juce::String& label,
                                    TsyganatorProcessor::PlayMode mode) {
        btn.setButtonText(label);
        btn.setClickingTogglesState(true);
        btn.onClick = [this, mode, &btn]() {
            // Immediate mutual exclusion — no waiting for timer
            playOffButton.setToggleState(false, juce::dontSendNotification);
            playArpButton.setToggleState(false, juce::dontSendNotification);
            playSeqSynthButton.setToggleState(false, juce::dontSendNotification);
            playSeqSampleButton.setToggleState(false, juce::dontSendNotification);
            btn.setToggleState(true, juce::dontSendNotification);
            processor.setPlayMode(mode);
            repaint();  // Immediately update section dimming
        };
        addAndMakeVisible(btn);
    };
    setupPlayModeBtn(playOffButton, "Off", TsyganatorProcessor::ModeOff);
    setupPlayModeBtn(playArpButton, "Arp", TsyganatorProcessor::ModeArp);
    setupPlayModeBtn(playSeqSynthButton, "Seq Synth", TsyganatorProcessor::ModeSeqSynth);
    setupPlayModeBtn(playSeqSampleButton, "Seq Sample", TsyganatorProcessor::ModeSeqSample);

    // ========== Additional Buttons ==========

    tsyganizeButton.setButtonText("Tsyganize!");
    tsyganizeButton.setComponentID("tsyganize");
    tsyganizeButton.onClick = [this]() {
        processor.tsyganize();
        tsyganizeFlash = 1.0f;  // trigger glow flash animation
        repaint();
    };
    addAndMakeVisible(tsyganizeButton);

    initButton.setButtonText("Init");
    initButton.setComponentID("initBtn");
    initButton.onClick = [this]() { processor.loadPreset(0); };
    addAndMakeVisible(initButton);

    saveButton.setButtonText("Save");
    saveButton.onClick = [this]() {
        // P1-4: prompt for a name instead of overwriting "UserPreset" silently.
        // Default suggestion is the current preset name with a timestamp suffix.
        auto suggestion = juce::String("UserPreset_")
                        + juce::Time::getCurrentTime().formatted("%H%M%S");

        auto* aw = new juce::AlertWindow("Save preset",
                                         "Choose a name for your preset:",
                                         juce::MessageBoxIconType::QuestionIcon);
        aw->addTextEditor("name", suggestion, juce::String());
        aw->addButton("Save",   1, juce::KeyPress(juce::KeyPress::returnKey));
        aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        aw->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, aw](int result)
            {
                if (result == 1)
                {
                    auto name = aw->getTextEditorContents("name").trim();
                    if (name.isNotEmpty())
                    {
                        bool ok = processor.getPresetManager().savePreset(name);
                        if (!ok)
                        {
                            juce::AlertWindow::showAsync(
                                juce::MessageBoxOptions()
                                    .withIconType(juce::MessageBoxIconType::WarningIcon)
                                    .withTitle("Tsyganator")
                                    .withMessage("Failed to save preset (invalid name?)")
                                    .withButton("OK"),
                                nullptr);
                        }
                    }
                }
                delete aw;
            }), true);
    };
    addAndMakeVisible(saveButton);

    loadSampleButton.setButtonText("Load Sample");
    loadSampleButton.onClick = [this]() {
        // P1-3: restrict to formats actually supported by registerBasicFormats()
        // (WAV/AIFF). Previously listed MP3/FLAC silently failed at load time.
        fileChooser = std::make_unique<juce::FileChooser>(
            "Load Sample",
            juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
            "*.wav;*.aif;*.aiff");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode,
            [this](const juce::FileChooser& fc)
            {
                auto result = fc.getResult();
                if (result.existsAsFile())
                {
                    bool ok = processor.loadSampleFile(result);
                    if (!ok)
                    {
                        // Tell the user instead of failing silently
                        juce::AlertWindow::showAsync(
                            juce::MessageBoxOptions()
                                .withIconType(juce::MessageBoxIconType::WarningIcon)
                                .withTitle("Tsyganator")
                                .withMessage("Could not load sample. Only WAV and AIFF are supported.")
                                .withButton("OK"),
                            nullptr);
                    }
                }
            });
    };
    addAndMakeVisible(loadSampleButton);

    // unisonButton is a visible toggle button positioned in layoutRow2

    // ========== Presets ==========
    presetLabel.setText("Preset", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    presetPrevButton.setButtonText("<");
    presetPrevButton.onClick = [this]() {
        int idx = processor.getCurrentProgram();
        if (idx > 0)
        {
            processor.setCurrentProgram(idx - 1);
            presetCombo.setSelectedItemIndex(idx - 1, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(presetPrevButton);

    presetCombo.onChange = [this]() {
        int idx = presetCombo.getSelectedItemIndex();
        if (idx >= 0)
            processor.setCurrentProgram(idx);
    };
    addAndMakeVisible(presetCombo);

    presetNextButton.setButtonText(">");
    presetNextButton.onClick = [this]() {
        int idx = processor.getCurrentProgram();
        int numPrograms = processor.getNumPrograms();
        if (idx < numPrograms - 1)
        {
            processor.setCurrentProgram(idx + 1);
            presetCombo.setSelectedItemIndex(idx + 1, juce::dontSendNotification);
        }
    };
    addAndMakeVisible(presetNextButton);

    // ========== Status & Indicators ==========
    statusLabel.setText("Ready", juce::dontSendNotification);
    // statusLabel is not displayed — status info is painted directly in the bottom bar

    addAndMakeVisible(midiLedLabel);

    // ========== Sync initial UI state from processor (persistence on reopen) ==========
    {
        // Play mode buttons
        auto pm = processor.getPlayMode();
        playOffButton.setToggleState(pm == TsyganatorProcessor::ModeOff, juce::dontSendNotification);
        playArpButton.setToggleState(pm == TsyganatorProcessor::ModeArp, juce::dontSendNotification);
        playSeqSynthButton.setToggleState(pm == TsyganatorProcessor::ModeSeqSynth, juce::dontSendNotification);
        playSeqSampleButton.setToggleState(pm == TsyganatorProcessor::ModeSeqSample, juce::dontSendNotification);

        // Chorus toggles from APVTS
        int chorusIdx = (int)processor.apvts.getRawParameterValue("chorusMode")->load();
        chorusOffBtn.setToggleState(chorusIdx == 0, juce::dontSendNotification);
        chorusIBtn.setToggleState(chorusIdx == 1, juce::dontSendNotification);
        chorusIIBtn.setToggleState(chorusIdx == 2, juce::dontSendNotification);
        chorusIPlusIIBtn.setToggleState(chorusIdx == 3, juce::dontSendNotification);

        // Master dB label
        updateMasterDbLabel();
    }

    // ========== Tooltips (pro polish) ==========
    // Short, helpful descriptions on hover. These don't appear until the user
    // hovers ~700 ms, so they don't get in the way of normal use.
    tsyganizeButton.setTooltip("Randomize most parameters around the current preset for instant new sound design.");
    initButton.setTooltip("Reset to the first factory preset of the current mode.");
    saveButton.setTooltip("Save the current state as a user preset (prompts for a name).");
    modeButton.setTooltip("Switch between HARD BELGIAN MODE (dark, New Beat / EBM) and SAD ITALIAN MODE (bright, Italo Disco).");
    unisonButton.setTooltip("Unison: stacks all 6 voices on a single note with detuning for thickness. Costs polyphony.");
    vintageButton.setTooltip("Vintage: multi-stage saturation + vintage EQ + bass-mono + soft-knee glue. The post-synth polish chain.");
    lfoSyncButton.setTooltip("LFO Free vs tempo-synced to the host. Synced exposes musical divisions.");
    loadSampleButton.setTooltip("Load a WAV or AIFF sample for the 'Seq Sample' play mode.");
    playOffButton.setTooltip("Off: classic polyphonic keyboard play.");
    playArpButton.setTooltip("Arp: held notes are arpeggiated using the chosen Rate and Mode.");
    playSeqSynthButton.setTooltip("Seq Synth: the step sequencer drives the synth. Keyboard becomes a 303-style transpose.");
    playSeqSampleButton.setTooltip("Seq Sample: the step sequencer triggers the loaded sample. Keyboard still plays the synth.");
    seqRandButton.setTooltip("Randomize the step sequencer pattern using musical scales.");
    seqClearButton.setTooltip("Clear the entire step sequencer pattern.");
    seqGlideButton.setTooltip("Toggle glide on the selected step (legato into the next note).");
    seqAccentButton.setTooltip("Toggle accent on the selected step (boosted velocity).");
    seqNotePlusButton.setTooltip("Transpose the selected step up by one semitone.");
    seqNoteMinusButton.setTooltip("Transpose the selected step down by one semitone.");
    seqVelPlusButton.setTooltip("Raise the selected step velocity.");
    seqVelMinusButton.setTooltip("Lower the selected step velocity.");
    seqPatternCombo.setTooltip("Load a built-in pattern preset matching the current mode.");
    arpRateCombo.setTooltip("Arp note value (musical division). Synced to host tempo.");
    arpModeCombo.setTooltip("Arp pattern: how held notes are sequenced.");
    presetCombo.setTooltip("Browse presets for the current mode.");
    presetPrevButton.setTooltip("Previous preset.");
    presetNextButton.setTooltip("Next preset.");

    // ========== Register listeners and start timer ==========
    processor.addModeListener(this);
    startTimer(15); // ~67 Hz refresh

    syncMode();

    // Force the arp rate / mode combos to reflect their current parameter
    // value visually. The attachment normally does this but on some hosts
    // the initial sync happens before the combos are visible, leaving the
    // ComboBox showing "..." until the user clicks.
    {
        auto applyAttachmentValue = [this](juce::ComboBox& combo, const juce::String& paramId)
        {
            if (auto* p = processor.apvts.getRawParameterValue(paramId))
                combo.setSelectedItemIndex((int)p->load(), juce::dontSendNotification);
        };
        applyAttachmentValue(arpRateCombo,        "arpRate");
        applyAttachmentValue(arpModeCombo,        "arpMode");
        applyAttachmentValue(lfoWaveformCombo,    "lfoWaveform");
        applyAttachmentValue(lfoDestinationCombo, "lfoDestination");
        applyAttachmentValue(lfoSyncRateCombo,    "lfoSyncRate");
        applyAttachmentValue(osc2OctaveCombo,     "osc2Octave");
    }

    // Force full repaint to ensure all labels pick up their themed colors
    // (guards against JUCE deferred-paint timing where transparent initial color persists)
    repaint();

    // P36c v4 + P43 A — MULTI-DELAYED FULL REPAINTS to handle timing variance.
    // The user reported VST3 is now ~99% clean but AU on Ableton 12 still
    // shows ghost overlays at reopen. The AU bug is a Cocoa NSView layer
    // cache that occasionally composites stale content from before the
    // editor was reattached. We can't predict when the layer cache flushes,
    // so we fire FULL repaints at many staggered delays — whichever ones
    // land after the host has finished re-attaching the view clean the
    // stale frames. SafePointer guards against destruction.
    juce::Component::SafePointer<TsyganatorEditor> safeThis(this);
    juce::MessageManager::callAsync([safeThis]()
    {
        if (safeThis != nullptr) safeThis->repaint();
    });
    // Ramp: dense in the first second, sparser up to 5s — covers slow
    // hosts (Live with many plug-ins loaded) without burning CPU forever.
    const int delaysMs[] = { 30, 80, 160, 260, 380, 550, 800, 1200, 1800, 2500, 3500, 5000 };
    for (int d : delaysMs)
    {
        juce::Timer::callAfterDelay(d, [safeThis]()
        {
            if (safeThis != nullptr) safeThis->repaint();
        });
    }
    // P43 A: also flag the editor as "warming up" — for the first ~200
    // timer ticks (~3 s at 67 Hz) the timerCallback will issue a FULL
    // repaint every 6 ticks (~90 ms) instead of the usual sub-rect
    // repaints. Belt-and-braces with the delays above, specifically for
    // AU on Ableton 12 where the Cocoa layer cache misbehaves.
    warmupTicksRemaining = 200;
}

TsyganatorEditor::~TsyganatorEditor()
{
    stopTimer();
    processor.removeModeListener(this);
    setLookAndFeel(nullptr);
}

// =============================================================
// P36c — FULL-REPAINT GUARDS
// =============================================================
// JUCE's editor can be hidden/re-shown by the host (e.g. closing
// the plugin window in Ableton, then reopening it) WITHOUT being
// destroyed. When the editor becomes visible again, JUCE may only
// invalidate the rectangles previously marked dirty (the mascot
// repaint window from the timer, for instance) — leaving the rest
// of the editor as stale pixels or garbage. The user reported a
// big diagonal yellow/lavender wedge plus missing labels/headers,
// classic symptoms of partial repaint after a hide → show cycle.
//
// Forcing a full repaint() on every visibility change and parent
// hierarchy change eliminates that class of bug.
// =============================================================
void TsyganatorEditor::visibilityChanged()
{
    // P36c v2 — ALWAYS force a full repaint (don't guard with isShowing()).
    // The guard was preventing the repaint when the host fires this callback
    // BEFORE the visibility chain to the root window is established, leaving
    // the editor with a stale render.
    mascotScale = 1.0f;
    repaint();
}

void TsyganatorEditor::parentHierarchyChanged()
{
    // Editor re-attached to a parent (host plugin window). Always force a
    // full repaint — repaint() is safe to call even if not yet showing; it
    // simply marks the component dirty so the next visible cycle paints
    // from scratch.
    repaint();
}


void TsyganatorEditor::paint(juce::Graphics& g)
{
    bool isBelgian = processor.getMode() == TsyganatorProcessor::BelgianMode;

    // ============================================================
    // P28 — FULL PROCEDURAL REFONTE
    // ============================================================
    // The bg PNG is gone. Every static visual element is now painted
    // procedurally so there is one and only one source of truth:
    //   • Body background (gradient)
    //   • Top bar (gradient + typo logo)
    //   • Section CARDS (rounded rect + drop shadow + filled header bar
    //     + outline) wrapped exactly around each row's components
    //   • Slim ribbon at the bottom with version / mode / signature
    // The dynamic overlays (mascot, flashes, neon, sequencer grid,
    // MIDI LED, sample name) live below this section unchanged.
    // ============================================================

    // ----- P34 Theme palette — POSTER-INSPIRED VINTAGE -----
    // Belgian: warm gold + royal navy (Tintin-poster era).
    // Italian: warm dark plum + dusty rose (vintage Italo).
    // Every blue/yellow in Belgian is derived from one of two families
    // (gold or navy). Every plum/rose in Italian is derived from one of
    // two families (plum or rose). Aligns the visual identity.
    const auto bodyTop     = isBelgian ? juce::Colour(0xFFE8C928) : juce::Colour(0xFF1F1336);
    const auto bodyBot     = isBelgian ? juce::Colour(0xFFD2B324) : juce::Colour(0xFF120822);
    const auto cardTop     = isBelgian ? juce::Colour(0xFFF2D74A) : juce::Colour(0xFF3A1F52);
    const auto cardBot     = isBelgian ? juce::Colour(0xFFE0C232) : juce::Colour(0xFF1F0E30);
    const auto cardOutline = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFC85E92);
    const auto headerFill  = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFC85E92);
    const auto headerText  = isBelgian ? juce::Colour(0xFFEFD03A) : juce::Colour(0xFFFFF0E8);
    const auto accent      = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFC85E92);

    // ----- 1. BODY BACKGROUND -----
    g.fillAll(bodyTop);
    {
        juce::ColourGradient bodyGrad(bodyTop, 0.0f, 72.0f,
                                       bodyBot, 0.0f, 600.0f, false);
        g.setGradientFill(bodyGrad);
        g.fillRect(0, 72, 1360, 528);
    }

    // ----- 2. TOP BAR — poster-inspired royal navy / plum gradient -----
    {
        const auto topBarTop = isBelgian ? juce::Colour(0xFF254A9E) : juce::Colour(0xFF2A1942);
        const auto topBarBot = isBelgian ? juce::Colour(0xFF142C70) : juce::Colour(0xFF120822);

        juce::ColourGradient topGrad(topBarTop, 0.0f, 0.0f, topBarBot, 0.0f, 72.0f, false);
        g.setGradientFill(topGrad);
        g.fillRect(0, 0, 1360, 72);

        // Top edge highlight (subtle, catches light)
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRect(0, 0, 1360, 1);

        // Accent line at the bottom of the top bar
        g.setColour(accent.withAlpha(0.65f));
        g.fillRect(0, 70, 1360, 2);

        // ----- LOGO TSYGANATOR — pre-rendered PNG (P43 B) -----
        // The procedural drawText approach (P40) was clean but the user
        // wanted the punchier DJ TSYGAN poster typography (condensed heavy
        // caps, slight italic slant, hard accent shadow, baseline rule).
        // That style needs glyph-level control that drawText can't deliver,
        // so the logo is now baked into a high-DPI PNG (320×56, generated
        // from Outfit-ExtraBold with horizontal squeeze + shear in PIL).
        // One PNG per mode: the Belgian (gold + red) and Italian (rose +
        // orange) variants live in Resources/ and are loaded via BinaryData.
        // User-supplied custom TSYGANATOR PNG, recoloured + gradient per mode.
        // Source aspect 4:1. Bumped to 256×64 for more presence in the header.
        // Drawn at x=14, ends at x=270; mascot left edge ~280 → 10 px gap.
        // Box centre y = 4 + 32 = 36 (matches every top-bar element).
        const int logoBoxX = 14;
        const int logoBoxY = 4;
        const int logoBoxW = 256, logoBoxH = 64;
        const auto& logoImg = isBelgian ? logoBelgian : logoItalian;
        if (logoImg.isValid())
        {
            g.drawImage(logoImg, logoBoxX, logoBoxY, logoBoxW, logoBoxH,
                        0, 0, logoImg.getWidth(), logoImg.getHeight());
        }
        else
        {
            // Defensive fallback — never goes blank even if BinaryData fails.
            const auto fallbackFill = isBelgian ? juce::Colour(0xFFEFD03A)
                                                : juce::Colour(0xFFE8B0C8);
            g.setColour(fallbackFill);
            g.setFont(juce::Font(juce::FontOptions("Outfit", 32.0f, juce::Font::bold)));
            g.drawText("TSYGANATOR", logoBoxX + 8, logoBoxY,
                       logoBoxW - 8, logoBoxH, juce::Justification::centredLeft);
        }

        // Reset font for the rest of paint().
        g.setFont(juce::Font(juce::FontOptions("Outfit", 13.0f, juce::Font::bold)));
    }

    // ----- 3. SECTION CARDS -----
    // Each card: drop shadow → gradient body → filled header bar →
    // header title → top highlight → outline. Sized to wrap each row's
    // existing component cluster (no resized() changes required).
    auto drawCard = [&](juce::Rectangle<float> rect, const juce::String& title)
    {
        constexpr float cornerR = 4.0f;
        constexpr float headerH = 16.0f;

        // Drop shadow (single soft layer)
        g.setColour(juce::Colours::black.withAlpha(isBelgian ? 0.12f : 0.35f));
        g.fillRoundedRectangle(rect.translated(0.0f, 2.0f), cornerR);

        // Card body — vertical gradient
        juce::ColourGradient cardGrad(cardTop, rect.getX(), rect.getY(),
                                       cardBot, rect.getX(), rect.getBottom(), false);
        g.setGradientFill(cardGrad);
        g.fillRoundedRectangle(rect, cornerR);

        // Filled header bar (rounded at the top, squared at the bottom)
        juce::Rectangle<float> header(rect.getX(), rect.getY(), rect.getWidth(), headerH);
        g.setColour(headerFill);
        g.fillRoundedRectangle(header, cornerR);
        g.fillRect(header.getX(), header.getY() + headerH - cornerR,
                   header.getWidth(), cornerR);

        // Header title
        g.setFont(juce::Font(juce::FontOptions("Outfit", 10.5f, juce::Font::bold)));
        g.setColour(headerText);
        g.drawText(title, header.toNearestInt(), juce::Justification::centred);

        // Top highlight just under the header
        g.setColour(juce::Colours::white.withAlpha(0.10f));
        g.fillRect(rect.getX() + 1.0f, rect.getY() + headerH,
                   rect.getWidth() - 2.0f, 1.0f);

        // Card outline (last)
        g.setColour(cardOutline.withAlpha(isBelgian ? 0.55f : 0.70f));
        g.drawRoundedRectangle(rect, cornerR, 1.0f);
    };

    // Row 1 (y=80, h=150) — fader-based oscillators / filter / ADSRs
    drawCard({  14.0f,  80.0f, 312.0f, 150.0f }, "OSC-1");
    drawCard({ 334.0f,  80.0f, 318.0f, 150.0f }, "OSC-2");
    drawCard({ 658.0f,  80.0f, 220.0f, 150.0f }, "FILTER");
    drawCard({ 884.0f,  80.0f, 210.0f, 150.0f }, "FILTER ADSR");
    drawCard({1100.0f,  80.0f, 246.0f, 150.0f }, "AMP ADSR");

    // Row 2 (y=234, h=104) — P44 reflow:
    //   PERFORMANCE | LFO | VINTAGE | CHORUS | MASTER
    // Vintage and Chorus are SEPARATE cards (P39 merge was reverted —
    // user wanted clear independent sections with standard card headers).
    drawCard({  14.0f, 234.0f, 360.0f, 104.0f }, "PERFORMANCE");
    drawCard({ 383.0f, 234.0f, 280.0f, 104.0f }, "LFO");
    drawCard({ 672.0f, 234.0f, 190.0f, 104.0f }, "VINTAGE");
    drawCard({ 872.0f, 234.0f, 219.0f, 104.0f }, "CHORUS");
    drawCard({1100.0f, 234.0f, 246.0f, 104.0f }, "MASTER");

    // Row 3 (y=342, h=82) — play mode / sequencer controls / sample
    drawCard({  14.0f, 342.0f, 374.0f,  82.0f }, "PLAY MODE");
    drawCard({ 396.0f, 342.0f, 626.0f,  82.0f }, "SEQUENCER");
    drawCard({1030.0f, 342.0f, 316.0f,  82.0f }, "SAMPLE");

    // Row 4 (y=428, h=126) — step sequencer grid
    drawCard({  14.0f, 428.0f, 1332.0f, 126.0f }, "STEP SEQUENCER");

    // ----- 4. SLIM BOTTOM RIBBON (y=560..588) -----
    // P34 — ribbon colours align with the poster palette:
    //   Belgian: deep royal navy bg + gold text (same gold as the logo).
    //   Italian: deepest plum bg + warm rose text (same rose as the logo).
    {
        const auto ribbonCol  = isBelgian ? juce::Colour(0xFF142C70) : juce::Colour(0xFF120822);
        const auto ribbonText = isBelgian ? juce::Colour(0xFFEFD03A) : juce::Colour(0xFFE8B0C8);

        g.setColour(ribbonCol);
        g.fillRect(0, 560, 1360, 28);

        g.setColour(accent.withAlpha(0.55f));
        g.fillRect(0, 588, 1360, 1);

        g.setFont(juce::Font(juce::FontOptions("Outfit", 10.0f, juce::Font::bold)));

        // Left: version
        g.setColour(ribbonText.withAlpha(0.78f));
        g.drawText("TSYGANATOR v1.0", 20, 561, 240, 26, juce::Justification::centredLeft);

        // Center: mode name
        const auto modeName = isBelgian ? juce::String("HARD BELGIAN MODE")
                                        : juce::String("SAD ITALIAN MODE");
        g.setColour(ribbonText.withAlpha(0.60f));
        g.drawText(modeName, 480, 561, 400, 26, juce::Justification::centred);

        // Right: signature + voice specs (replaces wasted footer space)
        g.setColour(ribbonText.withAlpha(0.70f));
        g.drawText(juce::CharPointer_UTF8(
                       "Made with \xe2\x99\xa5 in Brussels & Roma  \xc2\xb7  6-voice poly  \xc2\xb7  Juno filter"),
                   820, 561, 520, 26, juce::Justification::centredRight);
    }

    // === MASCOT: Animated drawing ===
    // P27 — bumped from 38 → 46 base. With the scale max reduced to 1.10
    // (instead of 1.18), max outerR = 46·0.72·1.10 = 36.4 px, so the mascot
    // extends 36 ± 36.4 ≈ (0..72) — just touches both edges of the top bar
    // at peak audio but never overflows. Visibly bigger / present as a
    // brand element.
    {
        // P36: mascotCX moved 345 → 315 to tighten the brand pair
        // (logo + mascot). Combined with the logo shift x=18→23, the
        // visible gap between logo text end and mascot peak-scale left
        // edge goes from ~62 px to ~25 px (logo ends ≈ 255, mascot
        // peak left = 315 − 34.85 = 280). Tight pairing — they read
        // as one brand unit like in the reference posters.
        float mascotCX = 315.0f;
        float mascotCY = 36.0f;
        // P32: mSize trimmed 46→44 so even at peak scale 1.10 the mascot
        // stays a full pixel inside the 72 px top bar. Peak outer radius
        // (Belgian) = 44·0.72·1.10 = 34.85 → bottom edge y=70.85 (1.15 px
        // breathing inside the bar).
        float mSize    = 44.0f * mascotScale;

        if (isBelgian)
        {
            // Belgian: EU-style acid smiley — 12 orbiting gold stars on royal navy
            // P34: blue circle and stars now use the poster palette
            // (royal navy 0xFF1E3F8C + gold 0xFFEFD03A) so the mascot
            // visually aligns with every other navy/gold in the editor.
            float outerR = mSize * 0.72f;

            // Royal navy circle (matches the new top bar / header / accents)
            g.setColour(juce::Colour(0xFF1E3F8C));
            g.fillEllipse(mascotCX - outerR, mascotCY - outerR, outerR * 2.0f, outerR * 2.0f);

            // 12 orbiting gold 5-pointed stars (rotate with smileyAngle)
            float starOrbitR = outerR * 0.73f;
            float starSize = outerR * 0.14f;
            g.setColour(juce::Colour(0xFFEFD03A));   // gold matching logo / headerText

            for (int i = 0; i < 12; ++i)
            {
                float angle = smileyAngle + (float)i * (juce::MathConstants<float>::twoPi / 12.0f);
                float sx = mascotCX + std::cos(angle) * starOrbitR;
                float sy = mascotCY + std::sin(angle) * starOrbitR;

                // Draw 5-pointed star
                juce::Path star;
                for (int p = 0; p < 10; ++p)
                {
                    float r = (p % 2 == 0) ? starSize : starSize * 0.45f;
                    float a = -juce::MathConstants<float>::halfPi + (float)p * juce::MathConstants<float>::pi / 5.0f;
                    float spx = sx + std::cos(a) * r;
                    float spy = sy + std::sin(a) * r;
                    if (p == 0) star.startNewSubPath(spx, spy);
                    else        star.lineTo(spx, spy);
                }
                star.closeSubPath();
                g.fillPath(star);
            }

            // ============================================================
            // P33: ACID SMILEY — drawn from the user-supplied SVG for
            // pixel-perfect identity. We give the SVG drawable a square
            // bounding box centred on (mascotCX, mascotCY) sized so the
            // face circle inside the SVG matches our procedural faceR.
            //
            // SVG viewBox is "50 50 924 924" (924×924 around the face).
            // Inside, the face circle has radius 461.476 → 49.95% of the
            // viewBox. So if we want our visual face radius = faceR, the
            // drawable's bounding box should have side = faceR / 0.4995
            // ≈ faceR · 2.002 ≈ 2·faceR. Centring this square on the
            // mascot gives an exact 1:1 reproduction of the SVG.
            //
            // Fallback: if the SVG failed to load, we render a procedural
            // approximation (kept commented for reference / safety).
            {
                const float faceR  = outerR * 0.46f;
                const float boxSide = faceR * 2.004f;   // SVG's face / vb ratio compensation

                if (acidSmileyDrawable != nullptr)
                {
                    acidSmileyDrawable->drawWithin(
                        g,
                        juce::Rectangle<float>(mascotCX - boxSide * 0.5f,
                                               mascotCY - boxSide * 0.5f,
                                               boxSide, boxSide),
                        juce::RectanglePlacement::centred,
                        1.0f);
                }
                else
                {
                    // Procedural fallback (only if SVG failed to load)
                    g.setColour(juce::Colour(0xFFF6EB13));
                    g.fillEllipse(mascotCX - faceR, mascotCY - faceR,
                                  faceR * 2.0f, faceR * 2.0f);
                    g.setColour(juce::Colours::black);
                    g.drawEllipse(mascotCX - faceR, mascotCY - faceR,
                                  faceR * 2.0f, faceR * 2.0f, 1.2f);
                }
            }
        }
        else
        {
            // Italian: procedural 3D disco ball with Y-axis rotation.
            // P32: ballR ratio 0.42 → 0.60 of mSize so the visual mass
            // matches the Belgian smiley's blue circle (which is 0.72 of
            // mSize). Both mascots now read at similar weight in the top
            // bar. With mSize=44, ballR≈26.4 (peak ≈29) — comfortably
            // inside the 72 px bar.
            float ballR = mSize * 0.60f;

            // Glow behind disco ball — P34: aligned with the new dusty-rose
            // accent so the mascot harmonises with every other rose in the
            // editor (cards, header, ribbon, accents).
            juce::ColourGradient glow(juce::Colour(0xFFC85E92).withAlpha(0.35f), mascotCX, mascotCY,
                                       juce::Colour(0xFFC85E92).withAlpha(0.0f),
                                       mascotCX + ballR * 1.3f, mascotCY + ballR * 1.3f, true);
            g.setGradientFill(glow);
            g.fillEllipse(mascotCX - ballR * 1.2f, mascotCY - ballR * 1.2f,
                         ballR * 2.4f, ballR * 2.4f);

            // Drop shadow
            g.setColour(juce::Colours::black.withAlpha(0.3f));
            g.fillEllipse(mascotCX - ballR * 0.5f, mascotCY + ballR * 0.8f,
                         ballR * 1.0f, ballR * 0.2f);

            // 3D disco ball with current rotation
            draw3DDiscoBall(g, mascotCX, mascotCY, ballR, discoAngle);
        }
    }

    // === TSYGANIZE FLASH GLOW ===
    // P36c v2: removed saveState/reduceClipRegion/restoreState wrapping —
    // tBounds.expanded(12) is already inside the 0..72 top bar, so the
    // clip was redundant. Plus we suspect saveState/restoreState +
    // reduceClipRegion of leaving a stale clip on host close/reopen.
    if (tsyganizeFlash > 0.01f)
    {
        auto tBounds = tsyganizeButton.getBounds().toFloat();
        auto accentGlow = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFC85E92);

        // Wide outer glow
        g.setColour(accentGlow.withAlpha(tsyganizeFlash * 0.25f));
        g.fillRoundedRectangle(tBounds.expanded(12), 10.0f);
        // Inner bright glow
        g.setColour(accentGlow.withAlpha(tsyganizeFlash * 0.4f));
        g.fillRoundedRectangle(tBounds.expanded(4), 6.0f);
        // White flash core
        g.setColour(juce::Colours::white.withAlpha(tsyganizeFlash * 0.15f));
        g.fillRoundedRectangle(tBounds, 4.0f);
    }

    // === ACTION BUTTON FLASH GLOW (Rand, Clr, Glide, Accent, N±, V±) ===
    if (actionFlash > 0.01f && actionFlashBtn != nullptr && actionFlashBtn->isVisible())
    {
        auto aBounds = actionFlashBtn->getBounds().toFloat();
        auto flashCol = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFC85E92);

        // Outer glow
        g.setColour(flashCol.withAlpha(actionFlash * 0.3f));
        g.fillRoundedRectangle(aBounds.expanded(6), 5.0f);
        // White flash core
        g.setColour(juce::Colours::white.withAlpha(actionFlash * 0.2f));
        g.fillRoundedRectangle(aBounds, 3.0f);
    }

    // === MODE BUTTON border accent (P44: removed the outer halos that
    // were drawing as visible "circles around the button" in Italian mode).
    {
        auto modeBounds = modeButton.getBounds().toFloat().reduced(1.0f);
        float pillR = modeBounds.getHeight() / 2.0f;
        auto glowColour = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFC85E92);
        float pulseIntensity = 0.55f + 0.1f * std::sin(neonPulsePhase);
        g.setColour(glowColour.withAlpha(pulseIntensity));
        g.drawRoundedRectangle(modeBounds, pillR, 2.0f);
    }

    // Color references for dynamic elements
    auto accentColour   = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFC85E92);
    auto textColour     = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFE8B0C8);
    auto seqActive      = isBelgian ? juce::Colour(0xFFCC1122) : juce::Colour(0xFFC85E92);
    auto bodyDarkColour = isBelgian ? juce::Colour(0xFFD2B324) : juce::Colour(0xFF120822);

    // Layout constants — must match layoutRow4() and the row 4 card header bar.
    // P28: shifted down by 5 px so they sit BELOW the new filled header
    // bar (y=428..444). LEDs are at y=446, steps at y=455, last step ends
    // at y=551 (card ends at y=554).
    constexpr int SEQ_GRID_Y = 455;     // Step backgrounds start Y
    constexpr int SEQ_GRID_H = 96;      // Step height
    constexpr int SEQ_LED_Y  = 446;     // LED diodes Y (above step bg, below header)

    // === SEQUENCER STEP BACKGROUNDS, LEDs, NOTE NAMES, GLIDE/ACCENT ===
    int numSteps = juce::roundToInt(processor.apvts.getRawParameterValue("seqNumSteps")->load());
    if (numSteps < 1) numSteps = 1;
    if (numSteps > 16) numSteps = 16;
    int stepWidth = 1320 / numSteps;

    auto& seq = processor.getSequencer();

    for (int i = 0; i < numSteps; ++i)
    {
        int stepX = 25 + i * stepWidth;
        const auto& step = seq.getStep(i);

        // Step background — rounded, gradient fill, depth effect (HTML preview style)
        auto stepRect = juce::Rectangle<float>((float)stepX + 1, (float)SEQ_GRID_Y + 1,
                                                (float)(stepWidth - 4), (float)(SEQ_GRID_H - 2));
        if (step.active)
        {
            float velHeat = step.velocity;
            // Gradient fill for active steps (warm, velocity-mapped)
            juce::ColourGradient stepGrad(
                seqActive.withAlpha(0.4f + velHeat * 0.5f), stepRect.getX(), stepRect.getY(),
                seqActive.withAlpha(0.25f + velHeat * 0.3f), stepRect.getX(), stepRect.getBottom(), false);
            g.setGradientFill(stepGrad);
        }
        else
        {
            g.setColour(bodyDarkColour.withAlpha(0.15f));
        }
        g.fillRoundedRectangle(stepRect, 5.0f);

        // Subtle border
        g.setColour((step.active ? seqActive : textColour).withAlpha(step.active ? 0.3f : 0.08f));
        g.drawRoundedRectangle(stepRect, 5.0f, 0.7f);

        // Inner top highlight for depth (active steps)
        if (step.active)
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRoundedRectangle(stepRect.getX() + 2, stepRect.getY() + 1,
                                   stepRect.getWidth() - 4, 3.0f, 2.0f);
        }

        // Step edit flash overlay (when N+/N-/V+/V-/Accent/Glide is used)
        if (i == stepEditFlashIdx && stepEditFlash > 0.01f)
        {
            g.setColour(accentColour.withAlpha(stepEditFlash * 0.4f));
            g.fillRoundedRectangle(stepRect, 5.0f);
            g.setColour(juce::Colours::white.withAlpha(stepEditFlash * 0.2f));
            g.drawRoundedRectangle(stepRect, 5.0f, 2.0f);
        }

        // LED diode above each step (PNG images)
        {
            auto& ledImg = step.active ? (isBelgian ? ledOnBelgian : ledOnItalian)
                                       : (isBelgian ? ledOffBelgian : ledOffItalian);
            if (!ledImg.isNull())
            {
                int ledDrawSize = 7;
                int ledX = stepX + stepWidth / 2 - ledDrawSize / 2;
                g.drawImage(ledImg, ledX, SEQ_LED_Y, ledDrawSize, ledDrawSize,
                           0, 0, ledImg.getWidth(), ledImg.getHeight());
            }
        }

        // Selection indicator (user-selected step for editing — cyan/white border)
        if (i == selectedStep)
        {
            auto selColour = isBelgian ? juce::Colour(0xFF00CCFF) : juce::Colour(0xFFFFFFFF);
            // Outer glow
            g.setColour(selColour.withAlpha(0.25f));
            g.drawRoundedRectangle(stepRect.expanded(3.0f), 7.0f, 3.0f);
            // Inner sharp border
            g.setColour(selColour.withAlpha(0.85f));
            g.drawRoundedRectangle(stepRect, 5.0f, 2.0f);
        }

        // Playhead indicator (playing step — red/pink, only during playback)
        if (i == seq.getCurrentStep() && processor.isSequencerActive() && i != selectedStep)
        {
            auto playheadColour = isBelgian ? juce::Colour(0xFFFF2233) : juce::Colour(0xFFFF69B4);
            // Outer glow
            g.setColour(playheadColour.withAlpha(0.3f));
            g.drawRoundedRectangle(stepRect.expanded(2.0f), 6.0f, 3.0f);
            // Inner border
            g.setColour(playheadColour.withAlpha(0.8f));
            g.drawRoundedRectangle(stepRect, 5.0f, 2.0f);
        }
        // When playhead AND selection overlap, show combined highlight
        else if (i == seq.getCurrentStep() && processor.isSequencerActive() && i == selectedStep)
        {
            auto playheadColour = isBelgian ? juce::Colour(0xFFFF2233) : juce::Colour(0xFFFF69B4);
            g.setColour(playheadColour.withAlpha(0.4f));
            g.fillRoundedRectangle(stepRect, 5.0f);
        }

        // Step number at top
        {
            g.setFont(juce::Font(juce::FontOptions("JetBrains Mono", 8.0f, juce::Font::plain)));
            g.setColour(textColour.withAlpha(0.3f));
            g.drawText(juce::String(i + 1), stepX + 2, SEQ_GRID_Y + 4, stepWidth - 6, 10,
                       juce::Justification::centred);
        }

        // Note name — centered in step
        {
            g.setFont(juce::Font(juce::FontOptions("JetBrains Mono", 11.0f, juce::Font::bold)));
            if (step.active)
            {
                g.setColour(juce::Colours::white.withAlpha(0.95f));
                g.drawText(midiNoteName(step.note), stepX + 1, SEQ_GRID_Y + 34,
                           stepWidth - 4, 14, juce::Justification::centred);
            }
            else
            {
                g.setColour(textColour.withAlpha(0.35f));
                g.drawText("--", stepX + 1, SEQ_GRID_Y + 34,
                           stepWidth - 4, 14, juce::Justification::centred);
            }
        }

        // Velocity bar at bottom (proportional height indicator)
        if (step.active)
        {
            float barH = step.velocity * 20.0f;
            g.setColour(accentColour.withAlpha(0.5f));
            g.fillRoundedRectangle(stepRect.getX() + 4, stepRect.getBottom() - 4 - barH,
                                   stepRect.getWidth() - 8, barH, 2.0f);
        }

        // Glide indicator (bottom-left tag)
        if (step.glide && step.active)
        {
            g.setColour(accentColour.withAlpha(0.8f));
            g.fillRoundedRectangle(stepRect.getX() + 3, stepRect.getBottom() - 8,
                                   (float)juce::jmin(stepWidth / 3, 12), 5.0f, 2.0f);
        }

        // Accent indicator (bottom-right tag)
        if (step.accent && step.active)
        {
            g.setColour(seqActive.withAlpha(0.9f));
            float blockW = (float)juce::jmin(stepWidth / 3, 12);
            g.fillRoundedRectangle(stepRect.getRight() - 3 - blockW, stepRect.getBottom() - 8,
                                   blockW, 5.0f, 2.0f);
        }
    }

    // === BOTTOM BAR STATUS TEXT ===
    // NOTE: The bottom bar text ("Hover knobs for info...") is baked into the
    // background PNG. We do NOT paint it again here to avoid double/garbled text.
    // Only the mode name is dynamic and painted on the right side of the bar.

    // === MIDI activity LED ===
    // P43 D2: dynamic LED next to the mode pill. Lights up on every MIDI
    // note-on and decays smoothly (~180 ms). Drawn procedurally so it has
    // a real glow, not a flat PNG. The timer (~67 Hz) repaints this 20×20
    // rect every tick so the brightness curve is visually smooth.
    {
        auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
        auto lastNoteTime = processor.getLastNoteOnTime();
        const double dt = now - lastNoteTime;
        // 0..1 brightness, 1.0 at note-on, fades to 0 over ~180 ms.
        float brightness = (dt < 0.18) ? (float)(1.0 - dt / 0.18) : 0.0f;
        midiLedOn = brightness > 0.05f;

        // P44: cx=1062 (moved 23 px LEFT of the mode pill — was kissing the
        // pill border at 1085) and ONLY draw when brightness > 0.05 so the
        // LED is invisible at rest and only lights up on incoming MIDI.
        const float cx = 1062.0f, cy = 36.0f;
        const float r  = 6.5f;
        if (brightness > 0.05f)
        {
            const auto activeCol = isBelgian ? juce::Colour(0xFFFF3344)
                                             : juce::Colour(0xFFFFB0C8);
            // Outer halo
            g.setColour(activeCol.withAlpha(brightness * 0.55f));
            g.fillEllipse(cx - r - 5.0f, cy - r - 5.0f, (r + 5.0f) * 2.0f, (r + 5.0f) * 2.0f);
            g.setColour(activeCol.withAlpha(brightness * 0.85f));
            g.fillEllipse(cx - r - 2.0f, cy - r - 2.0f, (r + 2.0f) * 2.0f, (r + 2.0f) * 2.0f);
            // LED body
            g.setColour(activeCol);
            g.fillEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
            // Specular highlight
            g.setColour(juce::Colours::white.withAlpha(0.25f + brightness * 0.45f));
            g.fillEllipse(cx - r * 0.45f, cy - r * 0.55f, r * 0.6f, r * 0.45f);
        }
    }

    // === SAMPLE NAME in Sample panel ===
    {
        auto sampleName = processor.getSamplePlayer().getSampleName();
        if (sampleName.isNotEmpty())
        {
            auto textCol = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFE8B0C8);
            g.setFont(juce::Font(juce::FontOptions("Outfit", 10.0f, juce::Font::bold)));
            g.setColour(textCol.withAlpha(0.7f));
            g.drawText(sampleName, 1030, 398, 312, 16, juce::Justification::centred, true);
        }
    }

    // NOTE: Section dimming moved to paintOverChildren() so it draws ON TOP of
    // child components (buttons, knobs, labels) — not behind them.
}

void TsyganatorEditor::paintOverChildren(juce::Graphics& g)
{
    // === SECTION DIMMING (dim inactive sections based on play mode) ===
    // Off/Arp: dim sequencer (Row 3 + Row 4) + sample
    // Seq Synth: sample dimmed only
    // Seq Sample: nothing dimmed
    // Painted OVER children so it visually dims buttons, knobs, and labels too.
    {
        auto playMode = processor.getPlayMode();
        bool seqActive_dm = (playMode == TsyganatorProcessor::ModeSeqSynth ||
                             playMode == TsyganatorProcessor::ModeSeqSample);
        bool sampleActive_dm = (playMode == TsyganatorProcessor::ModeSeqSample);

        // P28 — single, subtle dim wash aligned exactly to the section
        // cards drawn in paint(). The card outline already delimits each
        // section, so no extra dashed ring is needed (it used to compete
        // with the outline and feel noisy). The wash alone communicates
        // "this section is inactive in the current play mode".
        const bool isBelgian = processor.getMode() == TsyganatorProcessor::BelgianMode;
        const auto dimCol = isBelgian
            ? juce::Colour(0xFF000000).withAlpha(0.22f)   // subtle darken on yellow
            : juce::Colour(0xFF000000).withAlpha(0.45f);  // stronger on dark

        auto paintDimmedZone = [&](juce::Rectangle<float> r)
        {
            g.setColour(dimCol);
            g.fillRoundedRectangle(r, 4.0f);   // same corner radius as the card
        };

        // Dim SEQUENCER card (Row 3) when not in seq modes
        if (!seqActive_dm)
            paintDimmedZone({ 396.0f, 342.0f, 626.0f, 82.0f });

        // Dim STEP SEQUENCER card (Row 4) when not in seq modes
        if (!seqActive_dm)
            paintDimmedZone({ 14.0f, 428.0f, 1332.0f, 126.0f });

        // Dim SAMPLE card when not in sample mode
        if (!sampleActive_dm)
            paintDimmedZone({ 1030.0f, 342.0f, 316.0f, 82.0f });
    }

    // (P44: EFFECTS split-header overpaint removed — VINTAGE and CHORUS
    // are now drawn as two standard cards by drawCard() in paint().)
}

void TsyganatorEditor::resized()
{
    layoutRow1();
    layoutRow2();
    layoutRow3();
    layoutRow4();

    // Top bar: 72px tall, center=36. Elements centered vertically.
    // Layout: Logo+Mascot(left) | ◄ Preset ► | Save Init | Tsyganize! | LED Mode(right)
    presetLabel.setBounds(-100, -100, 1, 1);  // hidden

    // Small buttons (h=28): y = 36 - 14 = 22
    // Large buttons (h=42): y = 36 - 21 = 15

    // P25 — top bar reflow: the user reported the header felt "cramped" and
    // the mascot overlapped the logo. Now the layout has clearer breathing
    // (25 px between each group):
    //   logo       :   18..307
    //   mascot (~) :  340..380   (centre 360, max diameter ~64 even at peak)
    //   preset <   :  415..441
    //   preset cb  :  445..645   (200 wide)
    //   preset >   :  649..675
    //   save       :  708..750
    //   init       :  754..796
    //   tsyganize  :  828..1040  (212 wide — slight trim for breathing)
    //   midi led   : 1078..1090
    //   mode pill  : 1098..1350

    // Group 1: Preset selector (after mascot, with 25 px breathing)
    presetPrevButton.setBounds(415, 22, 26, 28);
    presetCombo.setBounds    (445, 22, 200, 28);
    presetNextButton.setBounds(649, 22, 26, 28);

    // Group 2: Save / Init actions
    saveButton.setBounds(708, 22, 42, 28);
    initButton.setBounds(754, 22, 42, 28);

    // Group 3: Tsyganize (main CTA, slightly slimmer to free up breathing space)
    tsyganizeButton.setBounds(828, 15, 212, 42);

    // Group 4: Mode indicator (far right)
    midiLedLabel.setBounds(1078, 30, 12, 12);
    modeButton.setBounds(1098, 15, 252, 42);

    // Bottom bar status is painted, not a component
}

void TsyganatorEditor::layoutRow1()
{
    // ROW1: card y=80..230 (h=150), header bar y=80..96 (h=16), body y=96..230 (h=134).
    // Content = label (14) + fader (106) = 120 px tall (label adjacent to fader).
    // P33: shift fader y 110 → 117 so the label–fader group is vertically
    // CENTERED in the body (top margin = bottom margin = 7 px) instead of
    // top-aligned to the header. labelAboveY = y − 14 → 103.
    int y = 117;
    int h = 106;

    // OSC1 (7 faders ~42px each) — labels ABOVE faders, below section title
    // P33: osc1X 20 → 24 — symmetric centering in 312W card body
    // (7×40 + 6×2 = 292 cluster width, 10 px margin each side).
    int osc1X = 24;
    int labelAboveY = y - 14;  // 14px label just above fader top (y=84)
    sawLevelSlider.setBounds(osc1X, y, 40, h);
    sawLevelLabel.setBounds(osc1X, labelAboveY, 40, 14);
    pulseLevelSlider.setBounds(osc1X + 42, y, 40, h);
    pulseLevelLabel.setBounds(osc1X + 42, labelAboveY, 40, 14);
    triangleLevelSlider.setBounds(osc1X + 84, y, 40, h);
    triangleLevelLabel.setBounds(osc1X + 84, labelAboveY, 40, 14);
    subLevelSlider.setBounds(osc1X + 126, y, 40, h);
    subLevelLabel.setBounds(osc1X + 126, labelAboveY, 40, 14);
    noiseLevelSlider.setBounds(osc1X + 168, y, 40, h);
    noiseLevelLabel.setBounds(osc1X + 168, labelAboveY, 40, 14);
    pulseWidthSlider.setBounds(osc1X + 210, y, 40, h);
    pulseWidthLabel.setBounds(osc1X + 210, labelAboveY, 40, 14);
    osc1VolumeSlider.setBounds(osc1X + 252, y, 40, h);
    osc1VolumeLabel.setBounds(osc1X + 252, labelAboveY, 40, 14);

    // OSC2 (6 faders + octave combo) — labels ABOVE
    // P33: osc2X 340 → 341 — fine centering tweak (1 px shift right).
    int osc2X = 341;
    osc2SawSlider.setBounds(osc2X, y, 40, h);
    osc2SawLabel.setBounds(osc2X, labelAboveY, 40, 14);
    osc2PulseSlider.setBounds(osc2X + 42, y, 40, h);
    osc2PulseLabel.setBounds(osc2X + 42, labelAboveY, 40, 14);
    osc2TriangleSlider.setBounds(osc2X + 84, y, 40, h);
    osc2TriangleLabel.setBounds(osc2X + 84, labelAboveY, 40, 14);
    osc2PWSlider.setBounds(osc2X + 126, y, 40, h);
    osc2PWLabel.setBounds(osc2X + 126, labelAboveY, 40, 14);
    osc2FineSlider.setBounds(osc2X + 168, y, 40, h);
    osc2FineLabel.setBounds(osc2X + 168, labelAboveY, 40, 14);
    osc2VolumeSlider.setBounds(osc2X + 210, y, 40, h);
    osc2VolumeLabel.setBounds(osc2X + 210, labelAboveY, 40, 14);
    // Octave selector — combo + label as a centered group (44px total height)
    {
        int octGroupH = 28 + 2 + 14;  // combo + gap + label = 44
        int octGroupY = y + (h - octGroupH) / 2;  // centered vertically
        osc2OctaveCombo.setBounds(osc2X + 256, octGroupY, 48, 28);
        osc2OctaveLabel.setBounds(osc2X + 250, octGroupY + 30, 56, 14);
    }

    // Filter section (3 rotary knobs centered in 220 px card body, x=658..878)
    // P33: filterX 668 → 671 for true centering — cluster width
    // 3×58 + 2×10 = 194; (220-194)/2 = 13 → first knob at 658+13 = 671.
    int filterKnobSize = 58;
    int filterKnobSpacing = 68;
    int filterX = 671;
    // Vertically center knob+label (74px) in usable panel area (96-230 = 134px)
    int fkY = 96 + (134 - (filterKnobSize + 16)) / 2;  // knob+gap+label centered in panel
    cutoffSlider.setBounds(filterX, fkY, filterKnobSize, filterKnobSize);
    cutoffLabel.setBounds(filterX, fkY + filterKnobSize + 2, filterKnobSize, 14);
    resonanceSlider.setBounds(filterX + filterKnobSpacing, fkY, filterKnobSize, filterKnobSize);
    resonanceLabel.setBounds(filterX + filterKnobSpacing, fkY + filterKnobSize + 2, filterKnobSize, 14);
    filterEnvAmountSlider.setBounds(filterX + filterKnobSpacing * 2, fkY, filterKnobSize, filterKnobSize);
    filterEnvAmountLabel.setBounds(filterX + filterKnobSpacing * 2, fkY + filterKnobSize + 2, filterKnobSize, 14);

    // Filter ADSR (4 faders centered in 210px card body at x=884..1094)
    // P33: filterAdsrX 917 → 919 for symmetric margins
    // (4×32 + 3×4 = 140 cluster width; (210-140)/2 = 35 → 884+35 = 919).
    int filterAdsrX = 919;
    filterAttackSlider.setBounds(filterAdsrX, y, 32, h);
    filterAttackLabel.setBounds(filterAdsrX - 8, labelAboveY, 48, 14);
    filterDecaySlider.setBounds(filterAdsrX + 36, y, 32, h);
    filterDecayLabel.setBounds(filterAdsrX + 36 - 8, labelAboveY, 48, 14);
    filterSustainSlider.setBounds(filterAdsrX + 72, y, 32, h);
    filterSustainLabel.setBounds(filterAdsrX + 72 - 8, labelAboveY, 48, 14);
    filterReleaseSlider.setBounds(filterAdsrX + 108, y, 32, h);
    filterReleaseLabel.setBounds(filterAdsrX + 108 - 8, labelAboveY, 48, 14);

    // Amp ADSR — same height as oscillators, centered within card (x=1100..1346, w=246)
    // P33: ampAdsrX 1151 → 1153 — symmetric margins (53 px each side).
    int ampAdsrX = 1153;
    ampAttackSlider.setBounds(ampAdsrX, y, 32, h);
    ampAttackLabel.setBounds(ampAdsrX - 8, labelAboveY, 48, 14);
    ampDecaySlider.setBounds(ampAdsrX + 36, y, 32, h);
    ampDecayLabel.setBounds(ampAdsrX + 36 - 8, labelAboveY, 48, 14);
    ampSustainSlider.setBounds(ampAdsrX + 72, y, 32, h);
    ampSustainLabel.setBounds(ampAdsrX + 72 - 8, labelAboveY, 48, 14);
    ampReleaseSlider.setBounds(ampAdsrX + 108, y, 32, h);
    ampReleaseLabel.setBounds(ampAdsrX + 108 - 8, labelAboveY, 48, 14);

    // Hide section labels (they're painted in the background PNG)
    osc1Label.setBounds(-100, -100, 1, 1);
    osc2Label.setBounds(-100, -100, 1, 1);
    filterLabel.setBounds(-100, -100, 1, 1);
    filterADSRLabel.setBounds(-100, -100, 1, 1);
    ampADSRLabel.setBounds(-100, -100, 1, 1);
}

void TsyganatorEditor::layoutRow2()
{
    // ROW2: card y=234..338 (h=104). Card header y=234..250 (h=16).
    // Card body y=250..338 (h=88).
    //
    // P30 — reorganised to PERFORMANCE | LFO | EFFECTS | CHORUS | MASTER.
    // P32 — VERTICAL CENTERING: knob (60) + gap (2) + label (14) = 76 px
    // content. Centred in 88 px body → top margin = 6 → knob top y = 256.
    // Knob CENTRE lands at 286, matching the Master 68 px knob centre
    // (252 + 34 = 286). All Row 2 knob centres line up visually.
    const int y           = 256;
    const int h           = 60;
    // P32: button TOP positioned so the button CENTRE lands at the knob
    // centre (y + h/2 = 286). Same trick for the chorus toggleY below.
    const int btnCenterY  = y + h / 2 - 14;          // 272 — button (h=28) centre at 286, aligned with knob

    // -------- PERFORMANCE card (x=14..374) --------
    // 5 slots × 60 + 4 gaps × 10 + 2 margins × 10 = 360
    {
        const int slotW = 60;
        const int gap   = 10;
        auto col = [&](int i) { return 14 + 10 + i * (slotW + gap); };
        unisonButton.setBounds        (col(0), btnCenterY, slotW, 28);
        unisonDetuneSlider.setBounds  (col(1), y, slotW, h);
        unisonDetuneLabel.setBounds   (col(1), y + h + 2, slotW, 14);
        keyTrackingSlider.setBounds   (col(2), y, slotW, h);
        keyTrackingLabel.setBounds    (col(2), y + h + 2, slotW, 14);
        portamentoSlider.setBounds    (col(3), y, slotW, h);
        portamentoLabel.setBounds     (col(3), y + h + 2, slotW, 14);
        // Tune migrated from EFFECTS
        globalFineTuneSlider.setBounds(col(4), y, slotW, h);
        globalFineTuneLabel.setBounds (col(4), y + h + 2, slotW, 14);
    }

    // -------- LFO card (x=383..663) --------
    // 4 columns × 60 + 3 gaps × 8 + 2 margins × 8 = 280
    {
        const int slotW = 60;
        const int gap   = 8;
        auto col = [&](int i) { return 383 + 8 + i * (slotW + gap); };
        // col 0: Rate (free) — overlapped by SyncRate combo when sync ON
        lfoRateSlider.setBounds   (col(0), y, slotW, h);
        lfoRateLabel.setBounds    (col(0), y + h + 2, slotW, 14);
        lfoSyncRateCombo.setBounds(col(0), y + 10, slotW, 22);
        lfoSyncLabel.setBounds    (col(0), y + 34, slotW, 14);
        // col 1: Depth knob
        lfoDepthSlider.setBounds  (col(1), y, slotW, h);
        lfoDepthLabel.setBounds   (col(1), y + h + 2, slotW, 14);
        // col 2: Sync button (top) + Wave combo (bottom) — stacked
        lfoSyncButton.setBounds   (col(2), y + 2,  slotW, 24);
        lfoWaveformCombo.setBounds(col(2), y + 32, slotW, 22);
        lfoWaveformLabel.setBounds(col(2), y + 56, slotW, 14);
        // col 3: Dest combo (bottom half only, top deliberately empty)
        lfoDestinationCombo.setBounds(col(3), y + 32, slotW, 22);
        lfoDestinationLabel.setBounds(col(3), y + 56, slotW, 14);
    }

    // -------- VINTAGE card (x=672..862, w=190) — P44 split from EFFECTS --------
    // Button (60) + gap (12) + knob (60) = 132 wide cluster. Margins
    // (190-132)/2 = 29 each side. Card-centred, button-centre at y=286.
    {
        const int slotW = 60;
        const int gap   = 12;
        const int x0    = 672 + 29;            // 701
        vintageButton.setBounds      (x0,                  btnCenterY, slotW, 28);
        vintageAmountSlider.setBounds(x0 + slotW + gap,    y,          slotW, h);
        vintageAmountLabel.setBounds (x0 + slotW + gap,    y + h + 2,  slotW, 14);
    }

    // -------- CHORUS card (x=872..1091, w=219) — P44 split from EFFECTS --------
    // 4 toggles × 36 + 3 gaps × 8 = 168 cluster. Card-centred at chorusX=898.
    // Toggle centre y=286 to match knob centres across Row 2.
    {
        const int toggleW = 36, toggleH = 36, toggleGap = 8;
        const int chorusCardX = 872, chorusCardW = 219;
        const int totalChorusW = toggleW * 4 + toggleGap * 3;
        const int chorusX = chorusCardX + (chorusCardW - totalChorusW) / 2;
        const int toggleY = 268;
        chorusOffBtn.setBounds    (chorusX,                                 toggleY, toggleW, toggleH);
        chorusIBtn.setBounds      (chorusX + (toggleW + toggleGap) * 1,     toggleY, toggleW, toggleH);
        chorusIIBtn.setBounds     (chorusX + (toggleW + toggleGap) * 2,     toggleY, toggleW, toggleH);
        chorusIPlusIIBtn.setBounds(chorusX + (toggleW + toggleGap) * 3,     toggleY, toggleW, toggleH);
    }

    // -------- MASTER card (x=1100..1346) --------
    // P29: knob 72→68 so the dB label fits inside the card.
    // P32: masterKnobY pinned to 252 (independent of the row `y` shift)
    // so the 68 px knob + 14 px dB label (84 px content) is centred in
    // the 88 px body — 2 px top margin, 2 px bottom margin — AND the
    // knob centre (252+34=286) aligns with the 60 px knobs in PERF/LFO/
    // EFFECTS (256+30=286).
    {
        const int masterX        = 1100;
        const int masterW        = 246;
        const int masterKnobSize = 68;
        const int masterKnobX    = masterX + (masterW - masterKnobSize) / 2;
        const int masterKnobY    = 252;   // hard-pinned: see comment above
        masterGainSlider.setBounds(masterKnobX, masterKnobY, masterKnobSize, masterKnobSize);
        masterGainLabel.setBounds(-100, -100, 1, 1);
        const int dbW = 100;
        masterDbLabel.setBounds(masterX + (masterW - dbW) / 2,
                                masterKnobY + masterKnobSize + 2, dbW, 14);
    }

    // Hide section labels (painted procedurally in paint())
    performanceLabel.setBounds(-100, -100, 1, 1);
    effectsLabel.setBounds(-100, -100, 1, 1);
    lfoLabel.setBounds(-100, -100, 1, 1);
    chorusLabel.setBounds(-100, -100, 1, 1);
    masterLabel.setBounds(-100, -100, 1, 1);
}

void TsyganatorEditor::layoutRow3()
{
    // ROW3: Y=342, H=82 (bottom=424). Section title ~342-358 (16px).
    // Usable area: 360-420 = 60px. Center 32px buttons → y = 360 + (60-32)/2 = 374.
    int y = 374;
    int h = 32;

    // Play mode buttons — within PLAY MODE panel (x=14, w=374, right edge=388).
    // 4 buttons + 2 combos. Cluster span: 0..366 relative to pmX → 366 wide.
    // P33: pmX 22 → 18 so left + right margins are symmetric at 4 px each
    // (the old +22 made the rightmost combo touch the card's right edge).
    const int pmX = 18;
    playOffButton.setBounds      (pmX,         y, 44, h);
    playArpButton.setBounds      (pmX +  48,   y, 44, h);
    playSeqSynthButton.setBounds (pmX +  96,   y, 70, h);
    playSeqSampleButton.setBounds(pmX + 170,   y, 76, h);

    // Arp combos — placed at right edge of the panel.
    // arpRateCombo = note value (1/4, 1/8, ...), arpModeCombo = pattern (Up/Down/...).
    arpLabel.setBounds    (-100, -100, 1, 1);  // not displayed (PLAY MODE label is in bg PNG)
    arpModeLabel.setBounds(-100, -100, 1, 1);  // ditto
    arpRateCombo.setBounds(pmX + 252, y + 2, 50, 28);
    arpModeCombo.setBounds(pmX + 306, y + 2, 60, 28);

    // Sequencer controls — within SEQUENCER card (x=396..1022, w=626).
    // P33: cluster shifted right by +2 — components used to span 398..1016
    // (left=2, right=6, asym) — now 400..1018 (left=4, right=4, ✓ symmetric).
    // Step count: minus / numeric display / plus (no slider)
    stepMinusButton.setBounds(400, y, 28, h);
    seqNumStepsLabel.setBounds(428, y, 34, h);
    stepPlusButton.setBounds(462, y, 28, h);
    sequencerLabel.setText("Steps", juce::dontSendNotification);
    sequencerLabel.setBounds(400, y + h + 2, 90, 11);

    // Swing & Gate knobs — square bounds to avoid arc overflow.
    int knobSz = h;
    int knobY = y - 2;
    seqSwingSlider.setBounds(500, knobY, knobSz + 6, knobSz + 6);
    seqSwingLabel.setBounds(498, knobY + knobSz + 10, knobSz + 10, 11);
    seqGateLengthSlider.setBounds(552, knobY, knobSz + 6, knobSz + 6);
    seqGateLengthLabel.setBounds(550, knobY + knobSz + 10, knobSz + 10, 11);

    // Action buttons
    seqRandButton.setBounds(604, y, 44, h);
    seqClearButton.setBounds(652, y, 44, h);
    seqGlideButton.setBounds(700, y, 52, h);
    seqAccentButton.setBounds(756, y, 56, h);

    // Stack N+/N- and V+/V- vertically
    int stackH = (h - 2) / 2;
    seqNotePlusButton.setBounds(818, y, 36, stackH);
    seqNoteMinusButton.setBounds(818, y + stackH + 2, 36, stackH);
    seqVelPlusButton.setBounds(858, y, 36, stackH);
    seqVelMinusButton.setBounds(858, y + stackH + 2, 36, stackH);

    // Pattern preset selector
    seqPatternCombo.setBounds(900, y + 2, 118, 24);

    // Sample section — within SAMPLE card (x=1030..1346, w=316).
    // P33: sampleCenterX recomputed from the actual card geometry
    // (was based on the old panel x=1026, w=320 ⇒ shifted 2 px left).
    int sampleCenterX = 1030 + 316 / 2;   // 1188 (was 1186)
    int loadW = 230;
    int loadH = h + 6;
    int loadY = y - 3;
    loadSampleButton.setBounds(sampleCenterX - loadW / 2, loadY, loadW, loadH);
}

void TsyganatorEditor::layoutRow4()
{
    // ROW4: Y=428, H=126 (428-554). Card header y=428..444 (16 px).
    // LEDs at y=446 (just below header). Grid at y=455.
    // Step buttons MUST match paint() SEQ_GRID_Y=455, SEQ_GRID_H=96.
    int y = 455;
    int h = 96;
    int numSteps = juce::roundToInt(processor.apvts.getRawParameterValue("seqNumSteps")->load());
    if (numSteps < 1) numSteps = 1;
    if (numSteps > 16) numSteps = 16;
    int stepWidth = 1320 / numSteps;

    for (int i = 0; i < numSteps && i < maxSequencerSteps; ++i)
    {
        int stepX = 25 + i * stepWidth;
        seqStepButtons[i].setBounds(stepX, y, stepWidth - 2, h);
    }

    // Hide remaining buttons
    for (int i = numSteps; i < maxSequencerSteps; ++i)
    {
        seqStepButtons[i].setBounds(-100, -100, 50, 50);
    }
}

void TsyganatorEditor::modeChanged(TsyganatorProcessor::SynthMode newMode)
{
    syncMode();
}

void TsyganatorEditor::syncMode()
{
    bool isBelgian = processor.getMode() == TsyganatorProcessor::BelgianMode;
    lookAndFeel.setMode(isBelgian);

    modeButton.setButtonText(isBelgian ? "Hard Belgian Mode" : "Sad Italian Mode");
    // Note: drawButtonText converts to uppercase for modeSwitch componentID

    // Update all label colors to match current theme — unified blue for Belgian
    auto labelCol = isBelgian ? juce::Colour(0xFF1E3F8C) : juce::Colour(0xFFE8B0C8);
    auto setLabelColor = [labelCol](juce::Label& l) {
        l.setColour(juce::Label::textColourId, labelCol);
    };
    // OSC1
    setLabelColor(sawLevelLabel); setLabelColor(pulseLevelLabel); setLabelColor(triangleLevelLabel);
    setLabelColor(subLevelLabel); setLabelColor(noiseLevelLabel); setLabelColor(pulseWidthLabel);
    setLabelColor(osc1VolumeLabel);
    // OSC2
    setLabelColor(osc2SawLabel); setLabelColor(osc2PulseLabel); setLabelColor(osc2TriangleLabel);
    setLabelColor(osc2PWLabel); setLabelColor(osc2FineLabel); setLabelColor(osc2VolumeLabel);
    setLabelColor(osc2OctaveLabel);
    // Filter
    setLabelColor(cutoffLabel); setLabelColor(resonanceLabel); setLabelColor(filterEnvAmountLabel);
    // Filter ADSR
    setLabelColor(filterAttackLabel); setLabelColor(filterDecayLabel);
    setLabelColor(filterSustainLabel); setLabelColor(filterReleaseLabel);
    // Amp ADSR
    setLabelColor(ampAttackLabel); setLabelColor(ampDecayLabel);
    setLabelColor(ampSustainLabel); setLabelColor(ampReleaseLabel);
    // Performance
    setLabelColor(unisonDetuneLabel); setLabelColor(keyTrackingLabel); setLabelColor(portamentoLabel);
    // Effects (P26 fix: vintageAmountLabel was missing — appeared invisible
    // yellow-on-yellow in Belgian mode)
    setLabelColor(globalFineTuneLabel);
    setLabelColor(vintageAmountLabel);
    // LFO
    setLabelColor(lfoRateLabel); setLabelColor(lfoDepthLabel);
    setLabelColor(lfoWaveformLabel); setLabelColor(lfoDestinationLabel);
    setLabelColor(lfoSyncLabel);
    // Master
    setLabelColor(masterDbLabel);
    // Seq
    setLabelColor(seqNumStepsLabel);
    setLabelColor(sequencerLabel);
    setLabelColor(seqSwingLabel);
    setLabelColor(seqGateLengthLabel);

    // Set ComboBox text colours to match theme (yellow on blue / pink on purple)
    auto comboTextCol = isBelgian ? juce::Colour(0xFFEFD03A) : juce::Colour(0xFFE8B0C8);
    auto setComboColour = [comboTextCol](juce::ComboBox& cb) {
        cb.setColour(juce::ComboBox::textColourId, comboTextCol);
    };
    setComboColour(presetCombo);
    setComboColour(osc2OctaveCombo);
    setComboColour(lfoWaveformCombo);
    setComboColour(lfoDestinationCombo);
    setComboColour(lfoSyncRateCombo);
    setComboColour(arpRateCombo);
    setComboColour(arpModeCombo);
    setComboColour(seqPatternCombo);

    // Update pattern combo — show only patterns matching current mode
    seqPatternCombo.clear();
    seqPatternCombo.addItem("-- Pattern --", 100);
    if (isBelgian)
    {
        for (int i = 0; i < StepSequencer::getNumBelgianPatterns(); ++i)
            seqPatternCombo.addItem(StepSequencer::getPattern(i).name, i + 1);
    }
    else
    {
        for (int i = StepSequencer::getFirstItalianPatternIndex();
             i < StepSequencer::getFirstItalianPatternIndex() + StepSequencer::getNumItalianPatterns(); ++i)
            seqPatternCombo.addItem(StepSequencer::getPattern(i).name, i + 1);
    }
    seqPatternCombo.setSelectedId(100, juce::dontSendNotification);

    // Update presets combo — grouped by category with section headings
    presetCombo.clear();
    const auto& presets = processor.getCurrentPresets();
    juce::String lastCategory;
    for (size_t i = 0; i < presets.size(); ++i)
    {
        juce::String cat(presets[i].category);
        if (cat.isNotEmpty() && cat != lastCategory)
        {
            presetCombo.addSectionHeading(cat);
            lastCategory = cat;
        }
        presetCombo.addItem(presets[i].name, (int)(i + 1));
    }
    presetCombo.setSelectedItemIndex(processor.getCurrentProgram(), juce::dontSendNotification);

    repaint();
}

void TsyganatorEditor::updateMasterDbLabel()
{
    float normValue = processor.apvts.getRawParameterValue("masterGain")->load();
    // masterGain range is 0.0 to 1.0 → map to dB display (-inf to 0 dB)
    float dbValue = (normValue > 0.001f) ? (20.0f * std::log10(normValue)) : -60.0f;
    masterDbLabel.setText(juce::String(dbValue, 1) + " dB", juce::dontSendNotification);
}

void TsyganatorEditor::timerCallback()
{
    // Update playhead (repaint sequencer area if changed)
    auto& seq = processor.getSequencer();
    int currentStep = seq.getCurrentStep();
    if (currentStep != lastDisplayedStep)
    {
        lastDisplayedStep = currentStep;
        repaint(juce::Rectangle<int>(10, 428, 1340, 130));
    }

    // Update MIDI LED
    auto now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    auto lastNoteTime = processor.getLastNoteOnTime();
    bool shouldBeOn = (now - lastNoteTime < 0.1);
    if (shouldBeOn != midiLedOn)
    {
        midiLedOn = shouldBeOn;
        repaint(juce::Rectangle<int>(1100, 28, 30, 30));
    }

    // Update step count numeric display
    {
        int stepCount = juce::roundToInt(processor.apvts.getRawParameterValue("seqNumSteps")->load());
        seqNumStepsLabel.setText(juce::String(stepCount), juce::dontSendNotification);
    }

    // Update master dB label
    updateMasterDbLabel();

    // Update play mode button highlighting
    auto playMode = processor.getPlayMode();
    playOffButton.setToggleState(playMode == TsyganatorProcessor::ModeOff, juce::dontSendNotification);
    playArpButton.setToggleState(playMode == TsyganatorProcessor::ModeArp, juce::dontSendNotification);
    playSeqSynthButton.setToggleState(playMode == TsyganatorProcessor::ModeSeqSynth, juce::dontSendNotification);
    playSeqSampleButton.setToggleState(playMode == TsyganatorProcessor::ModeSeqSample, juce::dontSendNotification);

    // Sync chorus toggle buttons from parameter
    {
        int chorusIdx = (int)processor.apvts.getRawParameterValue("chorusMode")->load();
        chorusOffBtn.setToggleState(chorusIdx == 0, juce::dontSendNotification);
        chorusIBtn.setToggleState(chorusIdx == 1, juce::dontSendNotification);
        chorusIIBtn.setToggleState(chorusIdx == 2, juce::dontSendNotification);
        chorusIPlusIIBtn.setToggleState(chorusIdx == 3, juce::dontSendNotification);
    }

    // Sync UNISON + SAT toggle buttons from parameters (P37: Crush removed)
    {
        bool unisonOn = processor.apvts.getRawParameterValue("unisonMode")->load() > 0.5f;
        unisonButton.setToggleState(unisonOn, juce::dontSendNotification);
        bool vintageOn = processor.apvts.getRawParameterValue("vintageMode")->load() > 0.5f;
        vintageButton.setToggleState(vintageOn, juce::dontSendNotification);
    }

    // Sync LFO sync button state
    {
        bool syncOn = juce::roundToInt(processor.apvts.getRawParameterValue("lfoSync")->load()) == 1;
        if (syncOn != lfoSyncButton.getToggleState())
        {
            lfoSyncButton.setToggleState(syncOn, juce::dontSendNotification);
            lfoSyncButton.setButtonText(syncOn ? "Sync" : "Free");
            lfoRateSlider.setVisible(!syncOn);
            lfoRateLabel.setVisible(!syncOn);
            lfoSyncRateCombo.setVisible(syncOn);
            lfoSyncLabel.setVisible(syncOn);
        }
    }

    // P1-5: Sync preset combo with processor's current program.
    // Without this, host-driven program changes (DAW automation) update the
    // engine but the UI keeps showing the previous preset name.
    {
        int cur = processor.getCurrentProgram();
        if (cur >= 0 && cur < presetCombo.getNumItems()
            && cur != presetCombo.getSelectedItemIndex())
        {
            presetCombo.setSelectedItemIndex(cur, juce::dontSendNotification);
        }
    }

    // === Tsyganize flash decay (~300ms fade) ===
    if (tsyganizeFlash > 0.01f)
    {
        tsyganizeFlash *= 0.88f;
        repaint(tsyganizeButton.getBounds().expanded(20));
    }
    else
    {
        tsyganizeFlash = 0.0f;
    }

    // === Sync Glide/Accent toggle state from SELECTED step (not playhead) ===
    {
        // Clamp selectedStep to valid range
        int numSteps = juce::roundToInt(processor.apvts.getRawParameterValue("seqNumSteps")->load());
        if (selectedStep >= numSteps) selectedStep = numSteps - 1;
        if (selectedStep < 0) selectedStep = 0;

        if (selectedStep < StepSequencer::MAX_STEPS)
        {
            const auto& stepData = seq.getStep(selectedStep);
            seqGlideButton.setToggleState(stepData.glide, juce::dontSendNotification);
            seqAccentButton.setToggleState(stepData.accent, juce::dontSendNotification);
        }
    }

    // === Step edit flash decay (~400ms fade) ===
    if (stepEditFlash > 0.01f)
    {
        stepEditFlash *= 0.88f;
        repaint(juce::Rectangle<int>(10, 428, 1340, 130));
    }
    else
    {
        stepEditFlash = 0.0f;
        stepEditFlashIdx = -1;
    }

    // === Action button flash decay (~300ms fade) ===
    if (actionFlash > 0.01f)
    {
        actionFlash *= 0.85f;
        if (actionFlashBtn != nullptr)
            repaint(actionFlashBtn->getBounds().expanded(10));
    }
    else
    {
        actionFlash = 0.0f;
        actionFlashBtn = nullptr;
    }

    // === Mascot scale reactive to audio peak ===
    // P36c v2: DOUBLE-CLAMPED. We clamp targetScale to a sane range
    // [0.9, 1.10] (was just jmin → 1.10, allowing huge negatives if
    // getPeakLevel ever returned NaN/inf/huge negative), and we ALSO
    // clamp the final mascotScale to [0.5, 1.5] so a single bad audio
    // sample can never make the mascot drawable explode to fill the
    // editor with body-colour (which we suspect was the wedge bug
    // pattern on host close/reopen).
    float peakLevel = processor.getPeakLevel();
    if (!std::isfinite(peakLevel)) peakLevel = 0.0f;  // NaN/inf guard
    float targetScale = juce::jlimit(0.9f, 1.10f, 1.0f + peakLevel * 0.10f);
    mascotScale = juce::jlimit(0.5f, 1.5f, mascotScale * 0.85f + targetScale * 0.15f);

    // === Mascot rotation animations (continuous) ===
    // Disco ball: slow rotation (~0.24 rad/s at ~67Hz timer)
    discoAngle += 0.004f;
    if (discoAngle > juce::MathConstants<float>::twoPi)
        discoAngle -= juce::MathConstants<float>::twoPi;

    // Smiley vinyl-spin: ~3s per revolution
    smileyAngle += 0.015f;  // gentle star orbit — ~7s per revolution
    if (smileyAngle > juce::MathConstants<float>::twoPi)
        smileyAngle -= juce::MathConstants<float>::twoPi;

    // Neon pulse on mode button: 2s cycle (~67Hz → 0.047 rad/frame)
    neonPulsePhase += 0.047f;
    if (neonPulsePhase > juce::MathConstants<float>::twoPi)
        neonPulsePhase -= juce::MathConstants<float>::twoPi;

    // P36c v7 + P41 — Sub-rect repaints only. OpenGL was reverted (caused
    // text corruption on Tahoe). The clean reopen behaviour is now handled
    // by visibilityChanged() + parentHierarchyChanged() which force full
    // repaints when the host re-attaches the editor, combined with JUCE
    // 8.0.12 which fixes the Ableton close-window crash (8.0.7) plus
    // accumulated Direct2D / paint stability patches.
    repaint(juce::Rectangle<int>(275, 0, 80, 72));   // mascot + stars
    repaint(modeButton.getBounds().expanded(12));    // neon pulse
    // P43 D2: MIDI activity LED at cx=1085, cy=37 — refresh every tick so
    // the brightness fade (note-on → 180 ms decay) renders smoothly.
    repaint(juce::Rectangle<int>(1045, 22, 34, 30));   // P44: LED moved to cx=1062

    // P43 A — warm-up: for the first ~3 s after the editor was constructed,
    // issue a FULL repaint every 6 ticks (~90 ms) to flush stale Cocoa
    // NSView layer content on AU/Ableton 12 reopen. Belt-and-braces with
    // the staggered delayed repaints set up in the constructor.
    if (warmupTicksRemaining > 0)
    {
        if ((warmupTicksRemaining % 6) == 0)
            repaint();
        --warmupTicksRemaining;
    }
}

void TsyganatorEditor::buttonClicked(juce::Button* /*button*/)
{
    // Intentionally empty: callbacks live in each button's onClick lambda
    // for clarity and per-button locality. Kept only for safety in case a
    // future override needs the central dispatch.
}

void TsyganatorEditor::comboBoxChanged(juce::ComboBox* /*comboBoxThatHasChanged*/)
{
    // Intentionally empty: APVTS ComboBoxAttachments handle all sync.
}

// =================================================================
//  3D DISCO BALL — real-time sphere renderer
//  10 latitude × 16 longitude mesh, Y-axis rotation, diffuse+specular
//  lighting, rim light, pink tint — ported from preview.html
// =================================================================

void TsyganatorEditor::draw3DDiscoBall(juce::Graphics& g, float cx, float cy, float R, float angle)
{
    const int latSteps = 10;
    const int lonSteps = 16;
    float pi = juce::MathConstants<float>::pi;

    // Light direction (normalized) — matching HTML: (-0.5, -0.6, 0.7)
    float lx = -0.5f, ly = -0.6f, lz = 0.7f;
    float lLen = std::sqrt(lx * lx + ly * ly + lz * lz);
    lx /= lLen; ly /= lLen; lz /= lLen;

    float cosA = std::cos(angle);
    float sinA = std::sin(angle);

    // Build face list: each face has 4 projected corners, depth, and color
    struct Face {
        float x[4], y[4];
        float depth;
        juce::Colour col;
    };
    std::vector<Face> faces;
    faces.reserve(latSteps * lonSteps);

    for (int la = 0; la < latSteps; ++la)
    {
        float theta0 = pi * la / latSteps;
        float theta1 = pi * (la + 1) / latSteps;
        float sinT0 = std::sin(theta0), cosT0 = std::cos(theta0);
        float sinT1 = std::sin(theta1), cosT1 = std::cos(theta1);

        for (int lo = 0; lo < lonSteps; ++lo)
        {
            float phi0 = 2.0f * pi * lo / lonSteps;
            float phi1 = 2.0f * pi * (lo + 1) / lonSteps;
            float sinP0 = std::sin(phi0), cosP0 = std::cos(phi0);
            float sinP1 = std::sin(phi1), cosP1 = std::cos(phi1);

            // 4 corners in 3D sphere coords
            float sx[4] = { sinT0 * cosP0, sinT0 * cosP1, sinT1 * cosP1, sinT1 * cosP0 };
            float sy[4] = { cosT0,          cosT0,          cosT1,          cosT1 };
            float sz[4] = { sinT0 * sinP0, sinT0 * sinP1, sinT1 * sinP1, sinT1 * sinP0 };

            // Apply Y-axis rotation
            float rx[4], rz[4];
            for (int i = 0; i < 4; ++i)
            {
                rx[i] = sx[i] * cosA + sz[i] * sinA;
                rz[i] = -sx[i] * sinA + sz[i] * cosA;
            }

            // Face center normal (average of 4 corners, post-rotation)
            float ncx = (rx[0] + rx[1] + rx[2] + rx[3]) * 0.25f;
            float ncy = (sy[0] + sy[1] + sy[2] + sy[3]) * 0.25f;
            float ncz = (rz[0] + rz[1] + rz[2] + rz[3]) * 0.25f;
            float nLen = std::sqrt(ncx * ncx + ncy * ncy + ncz * ncz);
            if (nLen > 0.0001f) { ncx /= nLen; ncy /= nLen; ncz /= nLen; }

            // Back-face culling
            if (ncz < 0.0f) continue;

            // Diffuse lighting
            float diff = juce::jmax(0.0f, ncx * lx + ncy * ly + ncz * lz);

            // Specular (power 8, multiplier 1.4)
            float reflZ = 2.0f * ncz * (ncx * lx + ncy * ly + ncz * lz) - lz;
            float spec = std::pow(juce::jmax(0.0f, reflZ), 8.0f) * 1.4f;

            // Base color: grey with pink tint
            float base = 70.0f + diff * 160.0f;
            int r_ch = juce::jlimit(0, 255, (int)(base + spec * 255.0f));
            int g_ch = juce::jlimit(0, 255, (int)(base * 0.92f + spec * 200.0f));
            int b_ch = juce::jlimit(0, 255, (int)(base * 0.95f + spec * 220.0f));

            // Project to 2D
            Face f;
            f.depth = (rz[0] + rz[1] + rz[2] + rz[3]) * 0.25f;
            f.col = juce::Colour((juce::uint8)r_ch, (juce::uint8)g_ch, (juce::uint8)b_ch);
            for (int i = 0; i < 4; ++i)
            {
                f.x[i] = cx + rx[i] * R;
                f.y[i] = cy + sy[i] * R;
            }
            faces.push_back(f);
        }
    }

    // Sort by depth (painter's algorithm — back to front)
    std::sort(faces.begin(), faces.end(), [](const Face& a, const Face& b) {
        return a.depth < b.depth;
    });

    // Draw faces
    for (auto& f : faces)
    {
        juce::Path quad;
        quad.startNewSubPath(f.x[0], f.y[0]);
        quad.lineTo(f.x[1], f.y[1]);
        quad.lineTo(f.x[2], f.y[2]);
        quad.lineTo(f.x[3], f.y[3]);
        quad.closeSubPath();

        g.setColour(f.col);
        g.fillPath(quad);

        // Subtle grid lines between facets
        g.setColour(juce::Colours::black.withAlpha(0.12f));
        g.strokePath(quad, juce::PathStrokeType(0.5f));
    }

    // Rim light overlay
    {
        float rimX = cx + R * 0.35f;
        float rimY = cy + R * 0.3f;
        float rimR = R * 0.6f;
        juce::ColourGradient rimGrad(juce::Colour(255, 220, 255).withAlpha(0.18f), rimX, rimY,
                                       juce::Colours::transparentWhite, rimX - rimR, rimY - rimR, true);
        g.setGradientFill(rimGrad);
        g.fillEllipse(cx - R, cy - R, R * 2.0f, R * 2.0f);
    }

    // Specular highlight overlay
    {
        float specX = cx - R * 0.28f;
        float specY = cy - R * 0.28f;
        float specR = R * 0.4f;
        juce::ColourGradient specGrad(juce::Colours::white.withAlpha(0.55f), specX, specY,
                                        juce::Colours::transparentWhite, specX + specR, specY + specR, true);
        g.setGradientFill(specGrad);
        g.fillEllipse(specX - specR * 0.8f, specY - specR * 0.8f, specR * 1.6f, specR * 1.6f);
    }
}
