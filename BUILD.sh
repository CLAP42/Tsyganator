#!/bin/bash
# ============================================================
#  TSYGANATOR — Build script
#  Lance cette commande depuis le dossier Tsyganator/
# ============================================================
#
#  Usage:
#    cd /chemin/vers/Tsyganator
#    chmod +x BUILD.sh
#    ./BUILD.sh
#
# ============================================================

set -e

echo "╔══════════════════════════════════════════╗"
echo "║     TSYGANATOR — Build VST/AU Plugin     ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# 1. Créer le dossier build s'il n'existe pas
mkdir -p build
cd build

# 2. Configurer avec CMake (Release pour performance)
echo "⚙️  Configuration CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# 3. Compiler (utilise tous les cœurs disponibles)
# --parallel : syntaxe CMake portable qui marche avec n'importe quel
# générateur (Xcode, Ninja, Make). Évite le bug "xcodebuild: invalid
# option '-j12'" sur Xcode 26.4+ qui est devenu strict sur les flags.
echo "🔨 Compilation en cours..."
cmake --build . --config Release --parallel

echo ""
echo "✅ Build terminé !"
echo ""
echo "Le plugin se trouve dans :"
echo "  build/Tsyganator_artefacts/Release/VST3/Tsyganator.vst3"
echo "  build/Tsyganator_artefacts/Release/AU/Tsyganator.component"
echo ""
echo "Pour installer dans ton DAW :"
echo "  cp -r build/Tsyganator_artefacts/Release/VST3/Tsyganator.vst3 ~/Library/Audio/Plug-Ins/VST3/"
echo "  cp -r build/Tsyganator_artefacts/Release/AU/Tsyganator.component ~/Library/Audio/Plug-Ins/Components/"
echo ""
echo "Puis relance ton DAW et scanne les plugins."
