# TSYGANATOR v1.0 — Windows Install (English)

*Belgian acid meets Italian melancholy.*

Follow these steps **in order**. Five minutes, no tech knowledge needed.

🇫🇷 **Français :** see `LISEZ-MOI.md` in this folder.

---

## 1. Unzip the download

You just downloaded a file called **`Tsyganator-v1.0-Windows.zip`** in your **Downloads** folder (`This PC` → `Downloads`).

### ⚠️ Important step (do this no matter what)

Windows tags anything from the internet with an invisible "lock" called *Mark of the Web*. That lock can prevent your DAW from loading the plugin. Remove it **before** unzipping:

1. **Right-click** `Tsyganator-v1.0-Windows.zip`
2. **Properties**
3. At the bottom, next to "Security", tick **"Unblock"**
4. **OK**

> 💡 **If you already unzipped without doing this:** no big deal — delete the unzipped folder (not the zip), redo the 4 steps above on the zip, then re-unzip. Takes 30 seconds.

### Now unzip

**Right-click** the zip → **Extract All…** → **Extract**. A folder called **`Tsyganator v1.0 Windows`** appears next to it. That's what we'll use.

Inside that folder there's a `Plugins` sub-folder with:

- 🎹 `Tsyganator.vst3` — for DAWs that speak **VST3** (Ableton, Cubase, Reaper, Bitwig, FL Studio, Studio One…)
- 💻 `Tsyganator.exe` — **standalone** version to play without a DAW *(optional)*

> 💡 On Windows, `Tsyganator.vst3` is a **folder**, not a file. That's normal — a Windows VST3 is a "bundle". Move the whole folder as-is, don't enter it.

---

## 2. Quit your DAW

If Ableton, FL Studio, Reaper, Cubase, etc. is open, **close it completely**. Otherwise it won't see the new plugin.

---

## 3. Put the plugin in the right folder

### VST3

1. Open **File Explorer** (Win + E)
2. In the address bar at the top, **copy-paste** this exact path:

   ```
   C:\Program Files\Common Files\VST3
   ```

3. Hit **Enter**. The folder opens (might be empty, that's fine).
4. **Drag and drop** the `Tsyganator.vst3` folder (from the unzipped folder) **into this window**.
5. Windows will ask for admin permission → click **Continue**.

> ⚠️ If the `VST3` folder doesn't exist inside `C:\Program Files\Common Files\`, create it manually (right-click → New → Folder → name it `VST3`).

> 💡 **No-admin alternative:** you can also drop the plugin in your user folder at `C:\Users\YourName\AppData\Local\Programs\Common\VST3\` (create the folders if they don't exist). Ableton, Reaper, Studio One, Cubase and Bitwig auto-scan that path. **FL Studio** users will need to add it manually (`Options → Manage plugins → Add path`).

### Standalone (optional)

`Tsyganator.exe` doesn't need to be installed — put it anywhere you like (Desktop, personal folder, etc.) and double-click to launch. It works like a regular app and lets you play the synth without opening a DAW.

---

## 4. Relaunch your DAW

On startup it'll scan new plugins (some DAWs need a manual rescan from the preferences). Tsyganator appears in the **instruments** list, under the manufacturer **`TsyganatorAudio`**.

✅ **Done.** Load it on a MIDI track and play.

---

## ⚠️ "Windows protected your PC" (SmartScreen)

The first time you launch `Tsyganator.exe`, Windows might show a blue screen:

> **Windows protected your PC. Microsoft Defender SmartScreen prevented an unrecognized app from starting.**

Relax, **it's not a virus**. Windows blocks any executable not digitally signed by a Microsoft-registered developer by default (and paying €300/year for a code-signing certificate just for a free plugin — no thanks).

To unblock:

1. Click **"More info"** (the small link under the title)
2. A **"Run anyway"** button appears at the bottom → click it

You'll never see this warning for this app again.

> 💡 For the VST3 plugin, you **won't** see a SmartScreen popup: your DAW loads it, not Windows directly.

---

## Requirements

- Windows 10 or Windows 11
- **64-bit (x64)** CPU — 32-bit not supported
- Not tested on Windows ARM (Surface Pro X et al.)

---

## Support the project

Tsyganator is free. If it helps you ship a track and you want to buy me a coffee:

**☕  https://paypal.me/tsyganator**

Bugs, ideas, preset suggestions, or a track you made with it — drop me a line:

📧  **romualdvarin@gmail.com**

Made with ♥ in Brussels & Roma.
