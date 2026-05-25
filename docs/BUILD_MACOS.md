# Build Tsyganator — macOS

Build natif sur ton Mac (Apple Silicon ou Intel). Produit : VST3, AU, Standalone.

## Prérequis

- **macOS 14+** (Sonoma ou plus récent, Tahoe 26 supporté)
- **Xcode** (App Store, gratuit) ou **Xcode Command Line Tools** (`xcode-select --install`)
- **CMake 3.22+** (`brew install cmake` si tu as Homebrew)
- **git** (pré-installé sur macOS)

Pas besoin d'installer JUCE — le CMake le récupère automatiquement depuis GitHub.

## Build rapide (commande unique)

Depuis le dossier `Tsyganator/` :

```bash
./BUILD.sh
```

Le script lance `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` puis
`cmake --build build --config Release` en utilisant tous les cœurs du CPU.

## Build manuel (si tu veux contrôler)

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release -- -j$(sysctl -n hw.ncpu)
```

## Sortie

Les artefacts sont produits dans `build/Tsyganator_artefacts/Release/` :

```
build/Tsyganator_artefacts/Release/
├── VST3/
│   └── Tsyganator.vst3       ← bundle VST3
├── AU/
│   └── Tsyganator.component   ← bundle AudioUnit (Logic, GarageBand)
└── Standalone/
    └── Tsyganator.app          ← appli autonome
```

## Installation manuelle

Le build **ne copie rien automatiquement** (`COPY_PLUGIN_AFTER_BUILD FALSE`).
Tu copies à la main vers les dossiers système :

```bash
# VST3 (Ableton, Bitwig, FL, Cubase, Reaper, Studio One…)
cp -r build/Tsyganator_artefacts/Release/VST3/Tsyganator.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/

# AU (Logic Pro, GarageBand)
cp -r build/Tsyganator_artefacts/Release/AU/Tsyganator.component \
      ~/Library/Audio/Plug-Ins/Components/

# Standalone (où tu veux)
cp -r build/Tsyganator_artefacts/Release/Standalone/Tsyganator.app \
      /Applications/
```

Relance ton DAW, lance un rescan des plugins. Si AU ne se voit pas dans Logic :

```bash
auval -v aumu Tsyn Tsyg
```

(devrait afficher `AU VALIDATION SUCCEEDED`)

## Clean rebuild

Si CMake/JUCE déconne :

```bash
rm -rf build
./BUILD.sh
```

(parfois `sudo rm -rf build` est nécessaire si Xcode a posé des locks)
