# TSYGANATOR v1.0 — macOS Install (English)

*Belgian acid meets Italian melancholy.*

Two install methods. **Pick the easy one: the 1-click installer.** The manual method is here only if you like knowing where your files go.

🇫🇷 **Français :** see `LISEZ-MOI.md` in this folder.

---

## ✨ Easy method — 1-click installer (recommended, 30 seconds)

### 1. Unzip the download

You just downloaded **`Tsyganator-v1.0-macOS.zip`** in your Downloads. Double-click it → a `Tsyganator v1.0` folder appears.

### 2. Quit your DAW

If Logic, Ableton, Reaper, etc. is open, close it completely. Otherwise it won't see the new plugin.

### 3. Run the installer

Inside the `Tsyganator v1.0` folder, you'll see a file called **`Installer Tsyganator.command`**.

➡️ **Right-click it** → **Open** → in the dialog that pops up, click **Open** a second time.

> ⚠️ **Why right-click instead of double-click?** Because macOS blocks anything from the internet that isn't signed by an Apple-registered developer. Right-click → Open is the only way to tell macOS "I downloaded this on purpose, let it run". **You only do this once for this file.**

A black Terminal window opens, the script runs for ~2 seconds, and shows green checkmarks ✅ confirming the VST3 and AU are installed. When it's done, press Enter to close.

### 4. Relaunch your DAW

Tsyganator appears in the **instruments** list, under the manufacturer **`TsyganatorAudio`**.

✅ **Done.** Load it on a MIDI track and play.

### Bonus — Standalone

The installer doesn't install the standalone version (for playing without a DAW). If you want it, drag `Tsyganator.app` (from `Plugins/`) into your `Applications` folder. On first launch, right-click → Open.

---

## 🛠 Manual method (if the installer misbehaves)

You can do the same thing by hand. Longer, but useful if you want to know where files go.

### 1. Unzip and identify the plugins

In `Tsyganator v1.0/Plugins/` you'll find:

- 🎹 `Tsyganator.vst3` — for VST3 DAWs (Ableton, Cubase, Reaper, Bitwig…)
- 🎹 `Tsyganator.component` — for AU DAWs (Logic, GarageBand)
- 💻 `Tsyganator.app` — standalone *(optional)*

> 💡 Not sure which format your DAW wants? Install both VST3 + AU. Doesn't hurt.

### 2. Quit your DAW

### 3. Move the plugins

**For VST3:** in Finder, `⌘ + Shift + G` → paste this path → Enter:

```
~/Library/Audio/Plug-Ins/VST3
```

Drop `Tsyganator.vst3` into the window. (If the `VST3` folder doesn't exist, create it.)

**For AU:** same idea with:

```
~/Library/Audio/Plug-Ins/Components
```

Drop `Tsyganator.component` in there.

**For Standalone:** drop `Tsyganator.app` in `/Applications`.

### 4. Remove macOS quarantine

This is the **invisible-but-critical** step the installer does automatically. macOS tags everything downloaded from the internet with a "quarantine" attribute that blocks DAWs from loading the plugin. Open **Terminal** (Applications → Utilities → Terminal), copy-paste these two lines one at a time (Enter after each):

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Tsyganator.vst3
```

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Tsyganator.component
```

If a line returns "No such file or directory", that means you didn't install that format. Ignore it.

> 💡 To verify it worked: `xattr ~/Library/Audio/Plug-Ins/VST3/Tsyganator.vst3` — if the line returns **nothing** (or nothing containing `quarantine`), you're good.

### 5. For the Standalone (Tsyganator.app)

Easier:
1. In `/Applications`, right-click `Tsyganator.app` → **Open**
2. macOS asks to confirm → **Open**

You'll never see the warning for this app again.

### 6. Relaunch your DAW

---

## Requirements

- macOS 14 (Sonoma) or later — Tahoe 26 supported
- **Apple Silicon** Mac (M1, M2, M3, M4)
- Intel Macs **not supported** in this release

---

## Support the project

Tsyganator is free. If it helps you ship a track and you want to buy me a coffee:

**☕  https://paypal.me/tsyganator**

Bugs, ideas, preset suggestions, or a track you made with it — drop me a line:

📧  **romualdvarin@gmail.com**

Made with ♥ in Brussels & Roma.
