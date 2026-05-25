#pragma once
#include <string>
#include <vector>

/**
 * TsyganatorPreset — Preset definition with all synth parameters
 * Two modes: HARD BELGIAN MODE (New Beat/EBM) and SAD ITALIAN MODE (Italo Disco)
 *
 * DESIGN PHILOSOPHY:
 * Belgian = DARK, aggressive, low cutoffs, harder envelopes, more sub, raw
 * Italian = BRIGHT, colorful, high cutoffs, softer envelopes, more chorus, warm
 *
 * RULES:
 * - No unison by default on any preset (user activates manually)
 * - LFO used where it adds character (not forced)
 * - Resonance always musical (never ear-fatiguing)
 */
struct TsyganatorPreset
{
    std::string name;
    std::string category;   // "Bass", "Lead", "Stab", "Pad", "Sequence", "Init"

    // OSC1
    float sawLevel;
    float pulseLevel;
    float subLevel;
    float noiseLevel;
    float pulseWidth;

    // OSC2
    float osc2Saw;
    float osc2Pulse;
    float osc2PW;
    int   osc2Octave;
    float osc2Fine;

    // Filter
    float cutoff;
    float resonance;
    float filterEnvAmount;

    // Filter ADSR
    float filterAttack;
    float filterDecay;
    float filterSustain;
    float filterRelease;

    // Amp ADSR
    float ampAttack;
    float ampDecay;
    float ampSustain;
    float ampRelease;

    // Chorus (0=Off, 1=I, 2=II, 3=I+II)
    int chorusMode;

    // Master
    float masterGain;

    // LFO
    float lfoRate        = 1.0f;
    float lfoDepth       = 0.0f;
    int   lfoWaveform    = 0;    // 0=Sine,1=Tri,2=Saw,3=Sq,4=S&H
    int   lfoDestination = 0;   // 0=Cutoff,1=PW,2=Pitch,3=Volume

    // Performance
    float unisonDetune   = 15.0f;
    float keyTracking    = 0.0f;
    float portamento     = 0.0f;

    // Triangle wave levels (at end for backward compat with existing presets)
    float triangleLevel  = 0.0f;
    float osc2Triangle   = 0.0f;

    // Vintage processor (Lemuria-style mastering chain)
    bool  vintageMode    = false;  // Off by default
    float vintageAmount  = 0.5f;   // 50% when enabled
};

// ============================================================
//  HARD BELGIAN MODE — New Beat / EBM / Dark Industrial
//  DARK, low cutoffs, aggressive, raw, hard envelopes
//  Resonance kept moderate to avoid ear fatigue
// ============================================================
inline std::vector<TsyganatorPreset> getBelgianPresets()
{
    return {
        // --- BASSES ---
        {
            "Frites Acides", "Bass",
            // 303 acid bass: saw through resonant filter, squelchy but controlled
            0.9f, 0.0f, 0.3f, 0.0f, 0.5f,     // OSC1: saw + sub for weight
            0.0f, 0.0f, 0.5f, 0, 0.0f,
            350.0f, 0.5f, 0.85f,                 // LOW cutoff, moderate reso → dark acid
            0.001f, 0.18f, 0.02f, 0.06f,         // snappy filter env
            0.001f, 0.28f, 0.05f, 0.06f,
            1, 0.85f,                             // chorus I + boosted gain for fullness
            // LFO: slow acid cutoff wobble
            0.4f, 0.12f, 0, 0
        },
        {
            "Boccaccio Sub Bomb", "Bass",
            // Deep sub explosion — felt more than heard
            0.0f, 0.0f, 1.0f, 0.0f, 0.5f,
            0.0f, 0.5f, 0.4f, -1, 0.0f,
            160.0f, 0.15f, 0.35f,
            0.001f, 0.4f, 0.1f, 0.15f,
            0.001f, 0.6f, 0.0f, 0.12f,
            1, 0.9f                               // chorus + big gain for sub impact
        },
        {
            "33 Tours de Force", "Bass",
            // 45RPM at 33RPM trick — slow, heavy, dark
            0.6f, 0.0f, 0.7f, 0.0f, 0.5f,
            0.5f, 0.0f, 0.5f, 0, 7.0f,
            220.0f, 0.3f, 0.35f,
            0.001f, 0.5f, 0.1f, 0.15f,
            0.001f, 0.7f, 0.0f, 0.12f,
            1, 0.9f,                              // chorus I for width + boosted gain
            // LFO: slow pitch drift for tape-warble
            0.15f, 0.06f, 0, 2
        },

        // --- LEADS ---
        {
            "Front 2-4-Biere", "Lead",
            // Aggressive EBM lead — hard, cutting, industrial
            0.0f, 1.0f, 0.0f, 0.0f, 0.25f,
            0.0f, 0.7f, 0.15f, 1, 0.0f,
            2800.0f, 0.25f, 0.65f,
            0.001f, 0.1f, 0.15f, 0.08f,
            0.001f, 0.05f, 0.7f, 0.1f,
            1, 0.9f,                              // chorus I + boosted
            // LFO: fast PWM buzz for industrial texture
            6.0f, 0.15f, 3, 1
        },
        {
            "Bruges La Morte", "Lead",
            // Dark gothic EBM — haunting, cold
            0.0f, 1.0f, 0.0f, 0.0f, 0.6f,
            0.0f, 0.8f, 0.4f, 0, -12.0f,
            1400.0f, 0.3f, 0.45f,
            0.01f, 0.25f, 0.3f, 0.35f,
            0.01f, 0.1f, 0.8f, 0.4f,
            2, 0.85f,                             // chorus II for eerie width
            // LFO: slow breathing cutoff sweep
            0.3f, 0.2f, 0, 0,
            15.0f, 0.0f, 0.02f                    // slight portamento for gothic slides
        },
        {
            "Mentasm Hoover", "Lead",
            // THE hoover — detuned chaos, aggressive
            0.9f, 0.0f, 0.4f, 0.0f, 0.5f,
            0.9f, 0.0f, 0.5f, 0, 25.0f,
            1200.0f, 0.25f, 0.25f,
            0.001f, 0.3f, 0.25f, 0.15f,
            0.001f, 0.4f, 0.7f, 0.25f,
            1, 0.85f,                             // chorus I for width + boosted
            // LFO: subtle pitch instability for hoover character
            2.5f, 0.04f, 0, 2,
            15.0f, 0.0f, 0.0f,                   // unison detune, key track, portamento
            0.0f, 0.0f,                           // triangle levels
            true, 0.6f                            // VINTAGE ON — adds warmth + compression to the chaos
        },

        // --- STABS ---
        {
            "Waffle Stab", "Stab",
            // Short industrial hit — punchy but with audible tail
            0.7f, 0.7f, 0.3f, 0.1f, 0.5f,
            0.5f, 0.5f, 0.5f, 1, 0.0f,
            3500.0f, 0.15f, 0.9f,
            0.001f, 0.08f, 0.0f, 0.10f,
            0.001f, 0.12f, 0.0f, 0.15f,
            1, 0.9f                               // boosted gain
        },
        {
            "Stomp des Ardennes", "Stab",
            // Metallic stomp — industrial percussion with ring-out
            0.0f, 0.6f, 0.0f, 0.5f, 0.15f,
            0.0f, 0.4f, 0.5f, -1, 0.0f,
            700.0f, 0.35f, 0.75f,
            0.001f, 0.10f, 0.0f, 0.12f,
            0.001f, 0.12f, 0.0f, 0.18f,
            1, 0.95f                              // chorus I + boosted for punch
        },

        // --- PADS ---
        {
            "Brouillard d'Anvers", "Pad",
            // Cold fog — detuned, slow, spacious, DARK
            0.5f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.5f, 0.0f, 0.5f, 0, 8.0f,
            800.0f, 0.08f, 0.08f,
            1.2f, 1.0f, 0.4f, 2.5f,
            0.8f, 0.5f, 0.7f, 3.0f,
            3, 0.65f,                             // chorus I+II for maximum width
            // LFO: very slow PW modulation for evolving texture
            0.08f, 0.25f, 0, 1,
            15.0f, 0.0f, 0.0f,                   // unison detune, key track, portamento
            0.0f, 0.0f,                           // triangle levels
            true, 0.45f                           // VINTAGE ON — adds depth and warmth to the fog
        },
        {
            "Nuclear Atomium", "Pad",
            // Harsh metallic radiation pad
            0.3f, 0.0f, 0.0f, 0.4f, 0.5f,
            0.0f, 0.6f, 0.1f, 1, 3.0f,
            1500.0f, 0.35f, 0.25f,
            0.4f, 0.7f, 0.35f, 1.2f,
            0.3f, 0.4f, 0.8f, 1.8f,
            2, 0.65f,                             // boosted gain
            // LFO: S&H on cutoff for random radiation crackle
            3.0f, 0.18f, 4, 0
        },

        // --- SEQUENCES ---
        {
            "Praline Sequencee", "Sequence",
            // Dark pulsing sequence
            0.8f, 0.0f, 0.3f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0, 0.0f,
            600.0f, 0.1f, 0.6f,
            0.001f, 0.18f, 0.05f, 0.10f,
            0.001f, 0.20f, 0.08f, 0.08f,
            1, 0.85f                              // chorus I + boosted
        },
        {
            "Sound of C Bell", "Sequence",
            // Metallic bell — dark bell, not sparkly
            0.0f, 0.8f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.6f, 0.3f, 2, 5.0f,
            5000.0f, 0.06f, 0.35f,
            0.001f, 0.4f, 0.0f, 0.25f,
            0.001f, 0.6f, 0.0f, 0.4f,
            1, 0.75f                              // boosted
        },

        // --- FM-INSPIRED (subtractive approximations of FM timbres) ---
        {
            "Les Caves de Gand", "Bass",
            // FM-style metallic bass: narrow pulse + detuned OSC2 at +7 semitones (fifth)
            0.0f, 0.9f, 0.9f, 0.0f, 0.08f,
            0.0f, 0.7f, 0.12f, 0, 7.0f,
            180.0f, 0.45f, 0.7f,
            0.001f, 0.12f, 0.0f, 0.05f,
            0.001f, 0.22f, 0.0f, 0.06f,
            1, 0.9f                               // chorus I + boosted
        },
        {
            "Tonnerre de Gand", "Bass",
            // FM thunder bass: two detuned saws + sub, extremely low cutoff
            0.7f, 0.0f, 1.0f, 0.0f, 0.5f,
            0.8f, 0.0f, 0.5f, -1, 3.0f,
            120.0f, 0.2f, 0.5f,
            0.001f, 0.35f, 0.08f, 0.1f,
            0.001f, 0.5f, 0.0f, 0.08f,
            1, 0.9f                               // chorus I + boosted
        },
        {
            "Manneken Piss Metal", "Stab",
            // FM bell-metal approximation: OSC2 at +5 semi (fourth) = inharmonic
            0.0f, 1.0f, 0.3f, 0.0f, 0.1f,
            0.0f, 0.8f, 0.08f, 1, 5.0f,
            450.0f, 0.5f, 0.85f,
            0.001f, 0.10f, 0.0f, 0.12f,
            0.001f, 0.15f, 0.0f, 0.22f,
            1, 0.9f,                              // chorus I + boosted
            // LFO: subtle pitch wobble for FM instability feel
            3.5f, 0.08f, 0, 2
        },
        {
            "Neon Judgment Day", "Pad",
            // FM-style dark drone: detuned oscillators at non-standard interval
            0.5f, 0.5f, 0.6f, 0.1f, 0.2f,
            0.4f, 0.4f, 0.25f, 0, 10.0f,
            280.0f, 0.3f, 0.2f,
            0.15f, 0.6f, 0.25f, 0.8f,
            0.1f, 0.4f, 0.6f, 1.2f,
            2, 0.8f,                              // boosted gain
            // LFO: slow PW modulation for movement
            0.8f, 0.3f, 0, 1,
            15.0f, 0.0f, 0.0f,
            0.0f, 0.0f,
            true, 0.55f                           // VINTAGE ON — dark drone benefits from saturation + EQ
        },

        // --- INIT ---
        {
            "Init Belgique", "Init",
            0.8f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0, 0.0f,
            5000.0f, 0.0f, 0.0f,
            0.01f, 0.2f, 0.7f, 0.3f,
            0.01f, 0.2f, 0.7f, 0.3f,
            0, 0.8f                               // slightly boosted init
        }
    };
}

// ============================================================
//  SAD ITALIAN MODE — Italo Disco / Spacesynth / Hi-NRG
//  BRIGHT, colorful, high cutoffs, lush chorus, softer envelopes
//  Warm, emotional, sparkling — the opposite of Belgian darkness
// ============================================================
inline std::vector<TsyganatorPreset> getItalianPresets()
{
    return {
        // --- BASSES ---
        {
            "Pasta al Basso", "Bass",
            // Classic Italo octave bass — bouncy, warm
            0.0f, 0.9f, 0.5f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0, 0.0f,
            2200.0f, 0.1f, 0.45f,
            0.001f, 0.22f, 0.12f, 0.10f,
            0.001f, 0.25f, 0.06f, 0.08f,
            1, 0.85f,                              // boosted gain
            // LFO: very subtle PW modulation for analog warmth
            1.2f, 0.08f, 0, 1
        },
        {
            "I Feel Moroder", "Bass",
            // Moroder pulsing bass — rhythmic but warm
            0.0f, 1.0f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.7f, 0.5f, -1, 0.0f,
            1200.0f, 0.4f, 0.8f,
            0.001f, 0.14f, 0.0f, 0.07f,
            0.001f, 0.20f, 0.05f, 0.06f,
            1, 0.9f                                // chorus I + boosted
        },
        {
            "Vespa a 200km/h", "Bass",
            // Funky bass — snappy, bright
            0.0f, 1.0f, 0.3f, 0.0f, 0.15f,
            0.3f, 0.0f, 0.5f, 0, 5.0f,
            2800.0f, 0.18f, 0.55f,
            0.001f, 0.10f, 0.03f, 0.06f,
            0.001f, 0.14f, 0.0f, 0.05f,
            1, 0.85f                               // chorus I + boosted
        },

        // --- LEADS ---
        {
            "Lacrime di Synthwave", "Lead",
            // Emotional crying lead — bright, soaring
            0.8f, 0.3f, 0.0f, 0.0f, 0.5f,
            0.6f, 0.0f, 0.5f, 0, -8.0f,
            6500.0f, 0.08f, 0.35f,
            0.01f, 0.2f, 0.35f, 0.25f,
            0.01f, 0.08f, 0.9f, 0.45f,
            3, 0.85f,                              // boosted gain
            // LFO: expressive vibrato
            5.2f, 0.1f, 0, 2,
            15.0f, 0.0f, 0.03f,                   // slight portamento for slides
            0.0f, 0.0f,                            // triangle levels
            true, 0.4f                             // VINTAGE ON — subtle warmth for emotional character
        },
        {
            "Gelato al Synth", "Lead",
            // Smooth warm lead
            0.9f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0, 0.0f,
            7000.0f, 0.04f, 0.3f,
            0.005f, 0.18f, 0.45f, 0.2f,
            0.005f, 0.08f, 0.85f, 0.25f,
            2, 0.85f,                              // boosted gain
            // LFO: gentle vibrato
            4.8f, 0.05f, 0, 2
        },
        {
            "Koto Visitors", "Lead",
            // Bright crystalline arpeggio — sparkling
            0.5f, 0.5f, 0.0f, 0.0f, 0.5f,
            0.4f, 0.4f, 0.5f, 1, 0.0f,
            9000.0f, 0.06f, 0.45f,
            0.001f, 0.12f, 0.08f, 0.07f,
            0.001f, 0.14f, 0.45f, 0.10f,
            3, 0.8f                                // boosted gain
        },

        // --- BRASS / STABS ---
        {
            "Mamma Mia Ottoni", "Brass",
            // Cheesy bright brass — thick, bold
            0.7f, 0.7f, 0.0f, 0.0f, 0.5f,
            0.6f, 0.5f, 0.5f, 0, 10.0f,
            5500.0f, 0.05f, 0.75f,
            0.006f, 0.1f, 0.45f, 0.12f,
            0.006f, 0.05f, 0.8f, 0.12f,
            1, 0.9f                                // boosted gain
        },
        {
            "Colosseo Strings", "Brass",
            // Lush string ensemble — warm, wide
            0.4f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.4f, 0.0f, 0.5f, 1, 6.0f,
            4500.0f, 0.02f, 0.12f,
            0.25f, 0.25f, 0.65f, 0.35f,
            0.15f, 0.12f, 0.9f, 0.5f,
            3, 0.75f,                              // boosted gain
            // LFO: slow volume tremolo for ensemble movement
            0.4f, 0.08f, 0, 3
        },

        // --- PADS ---
        {
            "Tramonto Romantico", "Pad",
            // Sunset pad — warm, full, dreamy
            0.8f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.7f, 0.0f, 0.5f, 0, 12.0f,
            5000.0f, 0.0f, 0.08f,
            0.4f, 0.4f, 0.65f, 1.0f,
            0.25f, 0.25f, 0.9f, 1.5f,
            3, 0.7f,                               // boosted gain
            // LFO: very slow cutoff sweep for movement
            0.06f, 0.12f, 0, 0,
            15.0f, 0.0f, 0.0f,
            0.0f, 0.0f,
            true, 0.5f                             // VINTAGE ON — warm sunset character
        },
        {
            "Nebbia di Milano", "Pad",
            // Foggy mysterious — still brighter than Belgian pads
            0.0f, 0.6f, 0.0f, 0.15f, 0.6f,
            0.0f, 0.5f, 0.4f, 0, -15.0f,
            2200.0f, 0.12f, 0.12f,
            0.8f, 0.7f, 0.45f, 2.0f,
            0.6f, 0.5f, 0.7f, 2.5f,
            2, 0.6f,                               // boosted gain
            // LFO: S&H sparkle through the fog
            1.5f, 0.1f, 4, 0
        },

        // --- SEQUENCES / ARPEGGIOS ---
        {
            "Rimini Aperol Arp", "Sequence",
            // Shimmering arp — bright, euphoric
            0.6f, 0.4f, 0.0f, 0.0f, 0.5f,
            0.3f, 0.3f, 0.5f, 1, 3.0f,
            8500.0f, 0.05f, 0.3f,
            0.001f, 0.14f, 0.12f, 0.08f,
            0.001f, 0.14f, 0.45f, 0.10f,
            3, 0.8f                                // boosted gain
        },
        {
            "Sabrina Sequenza", "Sequence",
            // Classic Italo sequence — melodic, bright
            0.5f, 0.5f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0, 0.0f,
            4500.0f, 0.1f, 0.5f,
            0.001f, 0.14f, 0.12f, 0.08f,
            0.001f, 0.16f, 0.40f, 0.08f,
            1, 0.9f                                // boosted gain
        },

        // --- OBERHEIM / SPACESYNTH INSPIRED ---
        {
            "Oberheim Tramonto", "Pad",
            // Classic OB-X pad: two fat detuned saws, wide chorus, warm filter
            0.9f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.9f, 0.0f, 0.5f, 0, 15.0f,
            4800.0f, 0.05f, 0.15f,
            0.08f, 0.3f, 0.6f, 0.5f,
            0.06f, 0.2f, 0.85f, 0.8f,
            3, 0.75f,                              // boosted gain
            // LFO: very slow filter sweep for movement
            0.3f, 0.15f, 0, 0,
            15.0f, 0.0f, 0.0f,
            0.0f, 0.0f,
            true, 0.55f                            // VINTAGE ON — analog warmth + glue for the OB-X feel
        },
        {
            "Laserdance Arp", "Sequence",
            // Spacesynth arpeggio: bright, crystalline, rapid-fire
            0.4f, 0.6f, 0.0f, 0.0f, 0.35f,
            0.3f, 0.5f, 0.4f, 1, 2.0f,
            9500.0f, 0.08f, 0.55f,
            0.001f, 0.10f, 0.05f, 0.05f,
            0.001f, 0.10f, 0.35f, 0.07f,
            1, 0.85f                               // boosted gain
        },
        {
            "Ottava Pulsante", "Bass",
            // Italo pulsating octave bass: two saws, octave apart
            0.9f, 0.0f, 0.4f, 0.0f, 0.5f,
            0.8f, 0.0f, 0.5f, -1, 0.0f,
            1800.0f, 0.15f, 0.65f,
            0.001f, 0.16f, 0.08f, 0.08f,
            0.001f, 0.18f, 0.05f, 0.06f,
            1, 0.85f                               // boosted gain
        },
        {
            "Cielo Stellato", "Lead",
            // Spacesynth lead: sync-style brightness, soaring, cosmic
            0.6f, 0.4f, 0.0f, 0.0f, 0.3f,
            0.5f, 0.3f, 0.2f, 1, 4.0f,
            7500.0f, 0.12f, 0.6f,
            0.001f, 0.15f, 0.25f, 0.1f,
            0.005f, 0.08f, 0.9f, 0.35f,
            3, 0.8f,                               // boosted gain
            // LFO: vibrato for expressiveness
            5.5f, 0.12f, 0, 2
        },

        // --- INIT ---
        {
            "Init Italia", "Init",
            0.8f, 0.0f, 0.0f, 0.0f, 0.5f,
            0.0f, 0.0f, 0.5f, 0, 0.0f,
            10000.0f, 0.0f, 0.0f,
            0.01f, 0.2f, 0.7f, 0.3f,
            0.01f, 0.2f, 0.7f, 0.3f,
            0, 0.8f                                // slightly boosted init
        }
    };
}

// Legacy combined presets
inline std::vector<TsyganatorPreset> getFactoryPresets()
{
    auto belgian = getBelgianPresets();
    auto italian = getItalianPresets();
    std::vector<TsyganatorPreset> all;
    all.insert(all.end(), belgian.begin(), belgian.end());
    all.insert(all.end(), italian.begin(), italian.end());
    return all;
}
