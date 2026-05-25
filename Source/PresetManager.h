#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/JunoPresets.h"
#include "DSP/StepSequencer.h"

/**
 * PresetManager — Handles saving/loading user presets to disk
 * Presets are stored as XML files in the user application data dir:
 *   macOS:    ~/Library/Application Support/Tsyganator/Presets/
 *   Windows:  %APPDATA%\Tsyganator\Presets\
 *   Linux:    ~/.config/Tsyganator/Presets/
 * `juce::File::userApplicationDataDirectory` handles the platform mapping.
 * Includes both synth parameters and sequencer pattern.
 */
class PresetManager
{
public:
    PresetManager(juce::AudioProcessorValueTreeState& vts, StepSequencer& seq)
        : apvts(vts), sequencer(seq)
    {
        // Create preset directory if it doesn't exist
        presetDir = juce::File::getSpecialLocation(
            juce::File::userApplicationDataDirectory)
            .getChildFile("Tsyganator")
            .getChildFile("Presets");
        presetDir.createDirectory();

        // Create subdirectories for organization
        presetDir.getChildFile("Belgian").createDirectory();
        presetDir.getChildFile("Italian").createDirectory();
        presetDir.getChildFile("User").createDirectory();

        refreshPresetList();
    }

    // Get the preset directory path
    juce::File getPresetDirectory() const { return presetDir; }

    // Save current state as a named preset
    bool savePreset(const juce::String& name, const juce::String& subfolder = "User")
    {
        // Validate preset name: reject empty or names with path separators
        if (name.isEmpty() || name.containsChar('/') || name.containsChar('\\'))
            return false;

        auto dir = presetDir.getChildFile(subfolder);
        dir.createDirectory();

        auto file = dir.getChildFile(name + ".tsyg");

        // Build XML
        auto xml = std::make_unique<juce::XmlElement>("TsyganatorPreset");
        xml->setAttribute("name", name);
        xml->setAttribute("version", "1.0");

        // Save synth parameters
        auto* paramsXml = xml->createNewChildElement("Parameters");
        auto state = apvts.copyState();
        paramsXml->addChildElement(state.createXml().release());

        // Save sequencer pattern
        auto* seqXml = xml->createNewChildElement("Sequencer");
        seqXml->setAttribute("numSteps", sequencer.getNumSteps());
        seqXml->setAttribute("swing", (double)sequencer.getSwing());
        seqXml->setAttribute("gateLength", (double)sequencer.getGateLength());

        for (int i = 0; i < StepSequencer::MAX_STEPS; ++i)
        {
            auto* stepXml = seqXml->createNewChildElement("Step");
            const auto& step = sequencer.getStep(i);
            stepXml->setAttribute("index", i);
            stepXml->setAttribute("note", step.note);
            stepXml->setAttribute("velocity", (double)step.velocity);
            stepXml->setAttribute("active", step.active);
            stepXml->setAttribute("glide", step.glide);
            stepXml->setAttribute("accent", step.accent);
        }

        bool success = xml->writeTo(file);
        if (success) refreshPresetList();
        return success;
    }

    // Load a preset from file
    bool loadPreset(const juce::File& file)
    {
        auto xml = juce::XmlDocument::parse(file);
        if (!xml || !xml->hasTagName("TsyganatorPreset"))
            return false;

        // Load synth parameters
        auto* paramsXml = xml->getChildByName("Parameters");
        if (paramsXml && paramsXml->getFirstChildElement())
        {
            auto tree = juce::ValueTree::fromXml(*paramsXml->getFirstChildElement());
            if (tree.isValid())
                apvts.replaceState(tree);
        }

        // Load sequencer pattern
        auto* seqXml = xml->getChildByName("Sequencer");
        if (seqXml)
        {
            sequencer.setNumSteps(seqXml->getIntAttribute("numSteps", 16));
            sequencer.setSwing((float)seqXml->getDoubleAttribute("swing", 0.0));
            sequencer.setGateLength((float)seqXml->getDoubleAttribute("gateLength", 0.5));

            for (auto* stepXml : seqXml->getChildWithTagNameIterator("Step"))
            {
                int idx = stepXml->getIntAttribute("index", 0);
                if (idx >= 0 && idx < StepSequencer::MAX_STEPS)
                {
                    sequencer.setStepNote(idx, stepXml->getIntAttribute("note", 60));
                    sequencer.setStepVelocity(idx, (float)stepXml->getDoubleAttribute("velocity", 0.8));
                    sequencer.setStepActive(idx, stepXml->getBoolAttribute("active", false));
                    sequencer.setStepGlide(idx, stepXml->getBoolAttribute("glide", false));
                    sequencer.setStepAccent(idx, stepXml->getBoolAttribute("accent", false));
                }
            }
        }

        currentPresetFile = file;
        currentPresetName = xml->getStringAttribute("name", file.getFileNameWithoutExtension());
        return true;
    }

    // Load preset by index from the scanned list
    bool loadPreset(int index)
    {
        if (index >= 0 && index < (int)userPresetFiles.size())
            return loadPreset(userPresetFiles[index]);
        return false;
    }

    // Delete a user preset
    bool deletePreset(const juce::File& file)
    {
        bool success = file.deleteFile();
        if (success) refreshPresetList();
        return success;
    }

    // Rename current preset
    bool renamePreset(const juce::String& newName)
    {
        if (currentPresetFile.existsAsFile())
        {
            auto newFile = currentPresetFile.getParentDirectory()
                                            .getChildFile(newName + ".tsyg");
            bool success = currentPresetFile.moveFileTo(newFile);
            if (success)
            {
                currentPresetFile = newFile;
                currentPresetName = newName;
                refreshPresetList();
            }
            return success;
        }
        return false;
    }

    // Scan preset directory for .tsyg files
    void refreshPresetList()
    {
        userPresetFiles.clear();
        userPresetNames.clear();

        juce::Array<juce::File> files;
        presetDir.findChildFiles(files, juce::File::findFiles, true, "*.tsyg");
        files.sort();

        for (const auto& f : files)
        {
            userPresetFiles.push_back(f);
            // Format: "Subfolder / Name"
            auto relative = f.getRelativePathFrom(presetDir);
            auto name = f.getFileNameWithoutExtension();
            auto parent = f.getParentDirectory().getFileName();
            if (parent != "Presets")
                userPresetNames.push_back(parent + " / " + name);
            else
                userPresetNames.push_back(name);
        }
    }

    // Getters
    const std::vector<juce::File>& getPresetFiles() const { return userPresetFiles; }
    const std::vector<juce::String>& getPresetNames() const { return userPresetNames; }
    const juce::String& getCurrentPresetName() const { return currentPresetName; }
    int getNumUserPresets() const { return (int)userPresetFiles.size(); }

    // Write factory presets to disk (first run)
    void installFactoryPresets()
    {
        auto belgianDir = presetDir.getChildFile("Belgian");
        auto italianDir = presetDir.getChildFile("Italian");

        if (belgianDir.getNumberOfChildFiles(juce::File::findFiles) == 0)
        {
            for (const auto& p : getBelgianPresets())
                writeFactoryPreset(p, belgianDir);
        }
        if (italianDir.getNumberOfChildFiles(juce::File::findFiles) == 0)
        {
            for (const auto& p : getItalianPresets())
                writeFactoryPreset(p, italianDir);
        }
        refreshPresetList();
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
    StepSequencer& sequencer;
    juce::File presetDir;
    juce::File currentPresetFile;
    juce::String currentPresetName;
    std::vector<juce::File> userPresetFiles;
    std::vector<juce::String> userPresetNames;

    void writeFactoryPreset(const TsyganatorPreset& p, const juce::File& dir)
    {
        auto xml = std::make_unique<juce::XmlElement>("TsyganatorPreset");
        xml->setAttribute("name", juce::String(p.name));
        xml->setAttribute("version", "1.0");
        xml->setAttribute("factory", true);

        auto* params = xml->createNewChildElement("SynthParams");
        params->setAttribute("sawLevel", (double)p.sawLevel);
        params->setAttribute("pulseLevel", (double)p.pulseLevel);
        params->setAttribute("subLevel", (double)p.subLevel);
        params->setAttribute("pulseWidth", (double)p.pulseWidth);
        params->setAttribute("cutoff", (double)p.cutoff);
        params->setAttribute("resonance", (double)p.resonance);
        params->setAttribute("filterEnvAmount", (double)p.filterEnvAmount);
        params->setAttribute("filterAttack", (double)p.filterAttack);
        params->setAttribute("filterDecay", (double)p.filterDecay);
        params->setAttribute("filterSustain", (double)p.filterSustain);
        params->setAttribute("filterRelease", (double)p.filterRelease);
        params->setAttribute("ampAttack", (double)p.ampAttack);
        params->setAttribute("ampDecay", (double)p.ampDecay);
        params->setAttribute("ampSustain", (double)p.ampSustain);
        params->setAttribute("ampRelease", (double)p.ampRelease);
        params->setAttribute("noiseLevel", (double)p.noiseLevel);
        params->setAttribute("osc2Saw", (double)p.osc2Saw);
        params->setAttribute("osc2Pulse", (double)p.osc2Pulse);
        params->setAttribute("osc2PW", (double)p.osc2PW);
        params->setAttribute("osc2Octave", p.osc2Octave);
        params->setAttribute("osc2Fine", (double)p.osc2Fine);
        params->setAttribute("chorusMode", p.chorusMode);
        params->setAttribute("masterGain", (double)p.masterGain);
        params->setAttribute("lfoRate", (double)p.lfoRate);
        params->setAttribute("lfoDepth", (double)p.lfoDepth);
        params->setAttribute("lfoWaveform", p.lfoWaveform);
        params->setAttribute("lfoDestination", p.lfoDestination);
        params->setAttribute("unisonDetune", (double)p.unisonDetune);
        params->setAttribute("keyTracking", (double)p.keyTracking);
        params->setAttribute("portamento", (double)p.portamento);
        params->setAttribute("triangleLevel", (double)p.triangleLevel);
        params->setAttribute("osc2Triangle", (double)p.osc2Triangle);

        auto file = dir.getChildFile(juce::String(p.name) + ".tsyg");
        xml->writeTo(file);
    }
};
