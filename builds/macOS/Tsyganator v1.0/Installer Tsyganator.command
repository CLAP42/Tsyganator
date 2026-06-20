#!/bin/bash
# ============================================================
#   TSYGANATOR — Installateur 1-clic pour macOS
# ============================================================
#
#   Double-clique sur ce fichier pour installer Tsyganator.
#
#   Si macOS affiche "ne peut pas être ouvert car le
#   développeur ne peut pas être vérifié", c'est normal :
#     1. Clic droit sur ce fichier dans Finder
#     2. Sélectionne "Ouvrir"
#     3. Confirme en cliquant "Ouvrir" dans la boîte de dialogue
#   C'est une seule fois — macOS retiendra ta confirmation.
#
# ============================================================

set -u

# Couleurs Terminal
BOLD='\033[1m'
GREEN='\033[1;32m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
CYAN='\033[1;36m'
RESET='\033[0m'

# Localiser le dossier du script (marche peu importe où il se trouve)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PLUGINS_DIR="$SCRIPT_DIR/Plugins"

# Bannière
clear
echo ""
echo -e "${CYAN}╔══════════════════════════════════════════════════════════╗${RESET}"
echo -e "${CYAN}║                                                          ║${RESET}"
echo -e "${CYAN}║${RESET}        ${BOLD}🎹  TSYGANATOR — Installation${RESET}                  ${CYAN}║${RESET}"
echo -e "${CYAN}║${RESET}        Belgian acid meets Italian melancholy            ${CYAN}║${RESET}"
echo -e "${CYAN}║                                                          ║${RESET}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════╝${RESET}"
echo ""

# Vérifier qu'on a bien le dossier Plugins à côté
if [ ! -d "$PLUGINS_DIR" ]; then
    echo -e "${RED}❌ ERREUR${RESET} : impossible de trouver le dossier 'Plugins/'."
    echo ""
    echo "Ce script doit rester DANS le dossier 'Tsyganator v1.0',"
    echo "au même niveau que le dossier 'Plugins'. Si tu l'as déplacé"
    echo "ailleurs, remets-le là où il était et redouble-clique."
    echo ""
    echo "Emplacement actuel du script :"
    echo "  $SCRIPT_DIR"
    echo ""
    echo "Appuie sur Entrée pour fermer..."
    read -r
    exit 1
fi

# ────────────────────────────────────────────────────────────
# Étape 1/2 — VST3
# ────────────────────────────────────────────────────────────
echo -e "${BOLD}Étape 1/2${RESET}  VST3 (Ableton, Cubase, Reaper, FL Studio, Bitwig…)"
echo ""

if [ -d "$PLUGINS_DIR/Tsyganator.vst3" ]; then
    mkdir -p "$HOME/Library/Audio/Plug-Ins/VST3"
    rm -rf "$HOME/Library/Audio/Plug-Ins/VST3/Tsyganator.vst3"
    cp -R "$PLUGINS_DIR/Tsyganator.vst3" "$HOME/Library/Audio/Plug-Ins/VST3/"
    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/Tsyganator.vst3" 2>/dev/null || true
    echo -e "          ${GREEN}✅ VST3 installé${RESET}"
    echo -e "             dans ~/Library/Audio/Plug-Ins/VST3/"
else
    echo -e "          ${YELLOW}⚠️  Pas de VST3 trouvé dans le dossier Plugins (ignoré)${RESET}"
fi

echo ""

# ────────────────────────────────────────────────────────────
# Étape 2/2 — Audio Unit
# ────────────────────────────────────────────────────────────
echo -e "${BOLD}Étape 2/2${RESET}  Audio Unit (Logic Pro, GarageBand)"
echo ""

if [ -d "$PLUGINS_DIR/Tsyganator.component" ]; then
    mkdir -p "$HOME/Library/Audio/Plug-Ins/Components"
    rm -rf "$HOME/Library/Audio/Plug-Ins/Components/Tsyganator.component"
    cp -R "$PLUGINS_DIR/Tsyganator.component" "$HOME/Library/Audio/Plug-Ins/Components/"
    xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/Tsyganator.component" 2>/dev/null || true
    echo -e "          ${GREEN}✅ Audio Unit installé${RESET}"
    echo -e "             dans ~/Library/Audio/Plug-Ins/Components/"
else
    echo -e "          ${YELLOW}⚠️  Pas de AU trouvé dans le dossier Plugins (ignoré)${RESET}"
fi

echo ""
echo -e "${GREEN}╔══════════════════════════════════════════════════════════╗${RESET}"
echo -e "${GREEN}║                                                          ║${RESET}"
echo -e "${GREEN}║${RESET}        ${BOLD}✅  Installation terminée !${RESET}                    ${GREEN}║${RESET}"
echo -e "${GREEN}║                                                          ║${RESET}"
echo -e "${GREEN}╚══════════════════════════════════════════════════════════╝${RESET}"
echo ""
echo -e "${BOLD}Prochaine étape :${RESET}"
echo ""
echo "  1. Lance ton DAW (Ableton, Logic, etc.)"
echo "  2. Si besoin, fais un rescan des plugins dans les"
echo "     préférences (Plug-Ins → Rescan)"
echo "  3. Cherche Tsyganator dans la liste des instruments"
echo -e "     Fabricant : ${BOLD}TsyganatorAudio${RESET}"
echo ""
echo -e "${BOLD}Bonus — Standalone${RESET} (pour jouer sans DAW) :"
echo "  Glisse 'Tsyganator.app' depuis le dossier Plugins/ vers"
echo "  ton dossier Applications. Au premier lancement, clic droit"
echo "  → Ouvrir → Ouvrir pour confirmer."
echo ""
echo -e "${BOLD}Bug ? Question ? Un track à me faire écouter ?${RESET}"
echo "  📧 romualdvarin@gmail.com"
echo "  ☕ https://paypal.me/tsyganator"
echo ""
echo "Made with ♥ in Brussels & Roma."
echo ""
echo "Appuie sur Entrée pour fermer cette fenêtre..."
read -r
