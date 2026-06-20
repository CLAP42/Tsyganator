# TSYGANATOR v1.0 — Installation Windows (Français)

*Belgian acid meets Italian melancholy.*

Bienvenue ! Suis les étapes ci-dessous **dans l'ordre**, dans 5 minutes c'est fait. Aucune connaissance technique requise.

---

## 1. Dézippe le téléchargement

Tu viens de télécharger un fichier qui s'appelle **`Tsyganator-v1.0-Windows.zip`** dans ton dossier **Téléchargements** (`Ce PC` → `Téléchargements`).

### ⚠️ Étape importante (à faire dans tous les cas)

Windows met un cadenas invisible — appelé "Mark of the Web" — sur tout ce qui vient d'Internet. Ce cadenas peut empêcher ton DAW de charger le plugin. Voici comment l'enlever **avant** de dézipper :

1. **Clic droit** sur `Tsyganator-v1.0-Windows.zip`
2. **Propriétés**
3. Tout en bas, à côté de "Sécurité", coche **"Débloquer"** (en anglais : *Unblock*)
4. **OK**

> 💡 **Si tu as déjà dézippé sans le faire** : pas grave, supprime le dossier dézippé (pas le zip), refais les 4 étapes ci-dessus sur le zip, puis re-dézippe. Ça prend 30 secondes.

### Maintenant dézippe

**Clic droit** sur le zip → **Extraire tout…** → **Extraire**. Un dossier nommé **`Tsyganator v1.0 Windows`** apparaît à côté. C'est ce dossier qu'on utilise.

À l'intérieur de ce dossier tu vas voir un sous-dossier `Plugins` qui contient :

- 🎹 `Tsyganator.vst3` — pour les DAW qui parlent **VST3** (Ableton, Cubase, Reaper, Bitwig, FL Studio, Studio One…)
- 💻 `Tsyganator.exe` — version **standalone** pour jouer sans DAW *(facultatif)*

> 💡 Sur Windows, `Tsyganator.vst3` est un **dossier** (et pas un fichier). C'est normal — un VST3 Windows c'est un "bundle". Tu le déplaces tel quel, sans entrer dedans.

---

## 2. Quitte ton DAW

Si Ableton, FL Studio, Reaper, Cubase, etc. est ouvert, **ferme-le complètement**. Sinon il ne verra pas le nouveau plugin.

---

## 3. Mets le plugin au bon endroit

### Pour le VST3

1. Ouvre l'**Explorateur de fichiers** (Win + E)
2. Dans la barre d'adresse en haut, **copie-colle** ce chemin exact :

   ```
   C:\Program Files\Common Files\VST3
   ```

3. Appuie sur **Entrée**. Le dossier s'ouvre (il est peut-être vide, c'est normal).
4. **Glisse-dépose** le dossier `Tsyganator.vst3` (depuis le dossier dézippé) **dans cette fenêtre**.
5. Windows va te demander une autorisation administrateur → clique **Continuer**.

> ⚠️ Si le dossier `VST3` n'existe pas dans `C:\Program Files\Common Files\`, crée-le manuellement (clic droit → Nouveau → Dossier → nomme-le `VST3`).

> 💡 **Alternative — pas besoin d'admin :** tu peux aussi mettre le plugin dans ton dossier utilisateur, à `C:\Users\TonNom\AppData\Local\Programs\Common\VST3\` (crée les dossiers s'ils n'existent pas). Ableton, Reaper, Studio One, Cubase et Bitwig scannent ce chemin automatiquement. Mais pour **FL Studio**, il faudra l'ajouter manuellement dans les options du DAW (`Options → Manage plugins → Add path`).

### Pour le Standalone (facultatif)

`Tsyganator.exe` n'a pas besoin d'être installé — tu peux le mettre où tu veux (Bureau, dossier perso, etc.) et le lancer en double-cliquant. Il fonctionne comme une appli normale et te permet de jouer du synthé sans ouvrir de DAW.

---

## 4. Re-lance ton DAW

Au démarrage, ton DAW va scanner les nouveaux plugins (sur certains DAW il faut forcer le rescan manuellement dans les préférences). Tsyganator apparaîtra dans la liste des **instruments**, sous le fabricant **`TsyganatorAudio`**.

✅ **C'est fait !** Tu peux le charger sur une piste MIDI et commencer à jouer.

---

## ⚠️ "Windows a protégé votre PC" (SmartScreen)

Quand tu lances `Tsyganator.exe` la première fois, Windows peut afficher un écran bleu :

> **Windows a protégé votre PC. Microsoft Defender SmartScreen a empêché le démarrage d'une application non reconnue.**

Pas de panique, **c'est pas un virus**. Windows bloque par défaut tout exécutable qui n'est pas signé numériquement par un développeur enregistré chez Microsoft (et payer un certificat 300 €/an juste pour un plugin gratuit, non merci).

Pour l'autoriser :

1. Clique sur **"Informations complémentaires"** (le petit lien sous le titre)
2. Un bouton **"Exécuter quand même"** apparaît en bas → clique dessus

Tu n'auras plus jamais ce message pour cette appli.

> 💡 Pour le plugin VST3, tu n'auras **pas** de message SmartScreen : c'est ton DAW qui le charge, pas Windows directement.

---

## Configuration minimale

- Windows 10 ou Windows 11
- Architecture **64-bit (x64)** — les CPU 32-bit ne sont pas supportés
- Pas testé sur Windows ARM (Surface Pro X et co.)

---

## Soutenir le projet

Tsyganator est gratuit. Si ça t'aide à faire des morceaux et que tu veux me payer un café :

**☕  https://paypal.me/tsyganator**

Bugs, idées, suggestions de presets, ou si tu veux me faire écouter ce que t'as fait avec :

📧  **romualdvarin@gmail.com**

Made with ♥ in Brussels & Roma.
