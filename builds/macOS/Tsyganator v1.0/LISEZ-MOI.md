# TSYGANATOR v1.0 — Installation macOS (Français)

*Belgian acid meets Italian melancholy.*

Deux façons d'installer. **Choisis la plus simple, l'installateur 1-clic.** La méthode manuelle est là juste si t'aimes savoir où vont tes fichiers.

🇬🇧 **English:** see `README.md` in this folder.

---

## ✨ Méthode rapide — Installateur 1-clic (recommandée, 30 secondes)

### 1. Dézippe le téléchargement

Tu viens de télécharger **`Tsyganator-v1.0-macOS.zip`** dans tes Téléchargements. Double-clique dessus → un dossier `Tsyganator v1.0` apparaît.

### 2. Quitte ton DAW

Si Logic, Ableton, Reaper, etc. est ouvert, ferme-le complètement. Sinon il ne verra pas le nouveau plugin.

### 3. Lance l'installateur

Dans le dossier `Tsyganator v1.0`, tu vois un fichier nommé **`Installer Tsyganator.command`**.

➡️ **Clic droit dessus** → **Ouvrir** → dans la boîte de dialogue qui apparaît, clique **Ouvrir** une seconde fois.

> ⚠️ **Pourquoi clic droit et pas double-clic ?** Parce que macOS bloque par défaut tout ce qui vient d'Internet et n'est pas signé par un développeur enregistré chez Apple. Le clic droit → Ouvrir, c'est la seule façon de dire à macOS "j'ai téléchargé ce truc en connaissance de cause, laisse-le tourner". **Tu ne le feras qu'une fois pour ce fichier.**

Une fenêtre Terminal noire s'ouvre, le script tourne pendant 2 secondes, et te montre des cases vertes ✅ pour confirmer que le VST3 et l'AU sont installés. Quand c'est fini, appuie sur Entrée pour fermer.

### 4. Relance ton DAW

Tsyganator apparaîtra dans la liste des **instruments**, sous le fabricant **`TsyganatorAudio`**.

✅ **C'est fait.** Tu peux le charger sur une piste MIDI et commencer à jouer.

### Bonus — Standalone

L'installateur n'installe pas la version standalone (pour jouer sans DAW). Si tu la veux, glisse manuellement `Tsyganator.app` (depuis `Plugins/`) dans ton dossier `Applications`. Au premier lancement, clic droit → Ouvrir.

---

## 🛠 Méthode manuelle (si l'installateur déconne)

Tu peux faire la même chose à la main. Plus long, mais utile pour comprendre où vont les fichiers.

### 1. Dézippe et identifie les plugins

Dans `Tsyganator v1.0/Plugins/` tu trouves :

- 🎹 `Tsyganator.vst3` — pour les DAW VST3 (Ableton, Cubase, Reaper, Bitwig…)
- 🎹 `Tsyganator.component` — pour les DAW AU (Logic, GarageBand)
- 💻 `Tsyganator.app` — version standalone *(facultatif)*

> 💡 Pas sûr du format dont ton DAW a besoin ? Installe les deux (VST3 + AU), ça mange pas de pain.

### 2. Quitte ton DAW

### 3. Déplace les plugins

**Pour le VST3 :** dans Finder, `⌘ + Maj + G` → colle ce chemin → Entrée :

```
~/Library/Audio/Plug-Ins/VST3
```

Glisse `Tsyganator.vst3` dans la fenêtre qui s'ouvre. (Si le dossier `VST3` n'existe pas, crée-le manuellement.)

**Pour l'AU :** même chose, mais avec :

```
~/Library/Audio/Plug-Ins/Components
```

Et tu glisses `Tsyganator.component`.

**Pour le Standalone :** glisse `Tsyganator.app` dans `/Applications`.

### 4. Enlève la quarantaine macOS

C'est l'étape **invisible mais critique** que l'installateur fait automatiquement. macOS marque tous les fichiers téléchargés d'Internet avec un attribut "quarantaine" qui empêche les DAW de les charger. Ouvre **Terminal** (Applications → Utilitaires → Terminal), copie-colle ces deux lignes une par une (Entrée après chacune) :

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Tsyganator.vst3
```

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Tsyganator.component
```

Si une ligne renvoie "No such file or directory", c'est que tu n'as pas installé ce format-là. Pas grave, ignore.

> 💡 Comment vérifier que ça a marché : `xattr ~/Library/Audio/Plug-Ins/VST3/Tsyganator.vst3` — si la ligne ne renvoie **rien** (ou rien qui contient `quarantine`), c'est bon.

### 5. Pour le Standalone (Tsyganator.app)

Plus simple :
1. Dans `/Applications`, clic droit sur `Tsyganator.app` → **Ouvrir**
2. macOS te demande confirmation → **Ouvrir**

Tu n'auras plus jamais ce message pour cette app.

### 6. Relance ton DAW

---

## Configuration minimale

- macOS 14 (Sonoma) ou plus récent — Tahoe 26 supporté
- Mac avec puce **Apple Silicon** (M1, M2, M3, M4)
- Mac Intel **non supporté** dans cette version

---

## Soutenir le projet

Tsyganator est gratuit. Si ça t'aide à faire des morceaux et que tu veux me payer un café :

**☕  https://paypal.me/tsyganator**

Bugs, idées, suggestions de presets, ou si tu veux me faire écouter ce que t'as fait avec :

📧  **romualdvarin@gmail.com**

Made with ♥ in Brussels & Roma.
