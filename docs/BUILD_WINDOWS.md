# Build Tsyganator — Windows

Tu **n'as pas besoin d'un PC Windows**. Tout se passe sur GitHub Actions : tu
pousses ton code, des serveurs Windows compilent le plugin pour toi, et tu
télécharges le `.vst3` prêt à distribuer.

Coût : **0 €** (gratuit pour repos publics, 2000 minutes/mois gratuites en
privé — un build prend ~3-5 min).

---

## Setup initial (à faire une seule fois)

### 1. Créer un compte GitHub

→ https://github.com/signup (gratuit, ~2 min)

### 2. Créer un repo pour Tsyganator

Sur github.com, clique sur le **+** en haut à droite → **New repository** :

- Repository name : `Tsyganator` (ou ce que tu veux)
- Description : `Hard Belgian × Sad Italian synth plugin`
- Public ou Private — peu importe, GitHub Actions est gratuit pour les deux
- **Ne coche pas** "Add a README" (le dossier en a déjà un implicite)
- Clique **Create repository**

GitHub t'affiche une page avec des instructions. Garde-la ouverte.

### 3. Pousser ton code

Depuis le terminal, dans le dossier `Tsyganator/` :

```bash
cd ~/Documents/Tsyganator

# Si tu n'as jamais utilisé git ici :
git init
git add .
git commit -m "Initial commit"
git branch -M main

# Relie ton dossier local au repo GitHub que tu viens de créer.
# Remplace TON_PSEUDO par ton vrai pseudo GitHub.
git remote add origin https://github.com/TON_PSEUDO/Tsyganator.git
git push -u origin main
```

GitHub va te demander ton login. Si ton compte a la 2FA activée, il te faut un
**Personal Access Token** au lieu de ton mot de passe :
Settings → Developer settings → Personal access tokens → Tokens (classic)
→ Generate new token → coche `repo` → copie le token et utilise-le comme
mot de passe lors du push.

---

## Le build automatique

Dès que ton premier `git push` arrive sur GitHub, le workflow
`.github/workflows/build-windows.yml` **se déclenche automatiquement**.

### Voir le build en cours

1. Va sur `github.com/TON_PSEUDO/Tsyganator`
2. Onglet **Actions** (en haut)
3. Tu vois la liste des runs. Clique sur le plus récent.
4. Clique sur le job **build-windows** pour voir les logs en direct.

Le build prend 3-5 min (la première fois est plus longue car JUCE doit être
téléchargé ; les builds suivants utilisent le cache et durent ~2 min).

### Récupérer le .vst3 Windows

Quand le run affiche un ✅ vert, tu scrolles en bas de la page du run et tu
trouves la section **Artifacts** :

- `Tsyganator-windows-vst3.zip` ← le plugin VST3
- `Tsyganator-windows-standalone.zip` ← l'app standalone Tsyganator.exe

Télécharge le zip, dézippe-le. Tu obtiens un dossier `Tsyganator.vst3`
(c'est normal — sur Windows un VST3 est un dossier-bundle, pas un fichier).

---

## Distribuer aux utilisateurs Windows

Sur ta page Lovable, ajoute un lien de téléchargement avec le zip
`Tsyganator-windows-vst3.zip`. Tes utilisateurs :

1. Dézippent
2. Copient le dossier `Tsyganator.vst3` dans
   `C:\Program Files\Common Files\VST3\` (chemin standard Windows)
3. Lancent leur DAW (Ableton/FL/Reaper/Cubase/Bitwig/Studio One) → rescan
   plugins → Tsyganator apparaît

**Note sur SmartScreen** : comme le .vst3 n'est pas signé numériquement,
Windows peut afficher un avertissement à la première utilisation. C'est
attendu pour un plugin gratuit non-signé. Si tu veux supprimer ce warning
plus tard, il faudra acheter un certificat Authenticode (~300€/an).

---

## Déclencher un build sans rien changer

Si tu veux re-builder sans modifier le code :

1. Va dans **Actions** → workflow **Build Windows VST3**
2. Clique **Run workflow** (bouton à droite)
3. Sélectionne la branche `main`
4. Clique **Run workflow**

---

## Releases versionnées (recommandé pour distribution)

Quand tu veux marquer une version officielle :

```bash
git tag v1.0.0
git push origin v1.0.0
```

Le push du tag déclenche un build dédié. L'artefact gardé 30 jours sera
identifiable par le numéro de version dans l'historique des runs.

---

## Workflow résumé

```
1. Tu modifies du code sur Mac          ← ton terminal
2. git add . && git commit -m "..."     ← tu valides
3. git push                              ← GitHub reçoit
4. GitHub Actions build sur Windows     ← automatique, ~3-5 min
5. Tu télécharges l'artefact .vst3      ← onglet Actions
6. Tu zippes pour Lovable                ← upload sur ta page
```

Tu peux faire ça depuis n'importe où, sans jamais toucher à un PC Windows.

---

## En cas de problème

- **Build rouge ❌** → clique sur le job, regarde quelle étape a planté
- **Erreur de compile MSVC** → souvent une différence subtile entre Clang
  (Mac) et MSVC (Windows). Copie l'erreur, je te débugge.
- **Cache JUCE corrompu** → dans Actions → onglet Caches → supprime
  `juce-Windows-*` → relance le build
- **Tu changes de version JUCE** → le hash du `CMakeLists.txt` change donc
  le cache est invalidé automatiquement ; pas d'action requise
