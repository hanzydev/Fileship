<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    Votre propre serveur ShareX et votre cloud de fichiers : une application Windows native, la synchronisation avec l'Explorateur de fichiers, une recherche par IA qui tourne sur votre serveur et une extension de navigateur.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Télécharger pour Windows</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Signaler un bug</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Proposer une fonctionnalité</a>
    ·
    <a href="SECURITY.md">Politique de sécurité</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <b>Français</b> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Une image déposée sur Fileship est importée et son lien copié, puis la recherche intelligente retrouve une photo à partir d'une description et affiche la description générée par l'IA" width="100%">
</p>

## 📦 Fonctionnalités

- Fichiers, dossiers imbriqués à partager avec d'autres utilisateurs, notes Markdown, liens courts et boîtes de dépôt, avec une corbeille pour tout ce que vous supprimez
- Sauvegardes de compte à télécharger et à restaurer, créées à la main ou planifiées
- Partage public avec mot de passe, date d'expiration, limite de vues et aperçus de liens soignés pour Discord et d'autres applications
- Une [IA](docs/ai.md) qui tourne sur votre serveur : recherche intelligente dans plus de 50 langues, descriptions, OCR et détection de contenu sensible, le tout facultatif
- Stockage des fichiers sur le disque local, tout service compatible S3, SFTP ou FTP
- Plusieurs utilisateurs avec autorisations et limites de stockage, clés d'accès, authentification à deux facteurs et [authentification unique](docs/configuration.md#single-sign-on) avec OpenID Connect
- Mises à jour sans interruption de service, en un clic ou planifiées
- Intégrations d'importation pour ShareX, macOS et Linux, l'[application Windows](docs/desktop.md) et une [application installable sur téléphone](docs/mobile.md) vers laquelle les applications Android peuvent partager
- Une [extension de navigateur](docs/browser-extension.md) qui importe des fichiers, des images et des captures d'écran et raccourcit les liens, [sur le Chrome Web Store](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh) pour Chrome et Edge, et [sur les modules Firefox](https://addons.mozilla.org/firefox/addon/fileship/) pour Firefox
- Des [webhooks](docs/webhooks.md) pour Discord, Slack et vos propres services
- Anglais, turc, allemand, espagnol, français, italien, portugais du Brésil, russe, azerbaïdjanais, japonais, coréen et chinois simplifié

## 🆚 Fileship face aux alternatives

|                                                     | Fileship                             | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| --------------------------------------------------- | ------------------------------------ | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX et autres outils d'envoi                     | ✅                                   | ✅                                          | ✅                                                 | ✅                                                  |
| Application de bureau pour capturer et enregistrer  | ✅ Windows                           | ❌                                          | ❌                                                 | ❌                                                  |
| Vos fichiers dans l'Explorateur de fichiers         | ✅ Fileship Harbor                   | ❌                                          | ❌                                                 | ❌                                                  |
| Extension de navigateur                             | ✅                                   | ❌                                          | ❌                                                 | ✅                                                  |
| Recherche IA, descriptions, OCR et contenu sensible | ✅ sur votre serveur                 | ❌                                          | ❌                                                 | ❌                                                  |
| Liens courts                                        | ✅                                   | ✅                                          | ❌                                                 | ✅                                                  |
| Notes ou pastes                                     | ✅ Markdown                          | ✅                                          | 🟡 envoi de texte                                  | ✅                                                  |
| Dossiers                                            | ✅ imbriqués, partagés entre comptes | ✅ imbriqués                                | ❌ tags                                            | 🟡 albums sur un seul niveau                        |
| Mots de passe, expiration et limites de vues        | ✅                                   | ✅                                          | ❌                                                 | 🟡 liens d'album qui expirent                       |
| Stockage                                            | Local, S3, SFTP, FTP                 | Local, S3                                   | Local, S3, FTP, Dropbox, Google Cloud              | Local, S3                                           |
| Authentification à deux facteurs et clés d'accès    | ✅                                   | ✅                                          | ❌                                                 | ❌                                                  |
| Authentification unique                             | ✅ OIDC                              | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhooks                                            | ✅ Discord, Slack, JSON signé        | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| Sauvegardes de compte restaurables et planifiées    | ✅                                   | 🟡 export                                   | 🟡 export                                          | ❌                                                  |
| Mises à jour depuis le tableau de bord              | ✅ sans interruption                 | ❌                                          | 🟡 avec interruption                               | ❌                                                  |
| Langues                                             | 12                                   | 12                                          | 31, certaines partielles                           | 1                                                   |
| Licence                                             | MIT                                  | MIT                                         | AGPL-3.0                                           | MIT                                                 |

Comparaison avec Zipline 4.8, XBackBone 3.8 et Chibisafe 6.7 en octobre 2026. Une information n'est plus à jour ? [Ouvrez une issue](https://github.com/hanzydev/Fileship/issues/new/choose) et elle sera corrigée.

## 📸 Captures d'écran

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Tableau de bord Fileship sur ordinateur" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Fichiers Fileship sur mobile" width="18%">
</p>

## 🖥️ Fileship pour Windows

Capturez, enregistrez et importez vers votre Fileship depuis la zone de notification, à la manière de ShareX, et parcourez vos fichiers dans l'Explorateur de fichiers avec Fileship Harbor. Nécessite Windows 11 64 bits version 24H2 ou ultérieure et un serveur Fileship 6.0.

**[Télécharger la dernière version](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [Fonctionnalités et installation](docs/desktop.md)

## 🚀 Démarrage

Il vous faut [Docker](https://www.docker.com/) avec [Docker Compose](https://docs.docker.com/compose/), 2 cœurs de processeur et 2 Go de RAM (4 Go ou plus avec l'IA, voir la [configuration requise](docs/installation.md#requirements)).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

Fileship est maintenant accessible sur `http://localhost:3000`. Connectez-vous avec `admin` / `password`.

> [!IMPORTANT]
> Changez le mot de passe dès votre première connexion (**Paramètres → Sécurité**).

Vous utilisez la version 5.x ? Suivez [Passer à la 6.0](docs/upgrading.md). Vous venez de Zipline, XBackBone ou Chibisafe ? [Importez vos comptes, fichiers et liens](docs/migrating.md).

## 📚 Documentation

- [Installation](docs/installation.md) : prérequis, navigateurs pris en charge et Fileship sur votre propre domaine
- [Configuration](docs/configuration.md) : paramètres du serveur, stockage, corbeille et variables d'environnement
- [Mises à jour](docs/updating.md) : fonctionnement des mises à jour et planification
- [Migrer depuis une autre application](docs/migrating.md) : importer un serveur Zipline, XBackBone ou Chibisafe
- [Fonctionnalités d'IA](docs/ai.md) : préréglages, modèles, recherche intelligente et contenu sensible
- [Fileship pour Windows](docs/desktop.md)
- [Fileship sur votre téléphone](docs/mobile.md) : installer l'application et partager depuis Android
- [Webhooks](docs/webhooks.md) : événements, formats, envoi et vérification de la signature
- [Télémétrie](docs/telemetry.md) : ce qui est collecté et comment la désactiver

## 🧰 Conçu avec

![Nuxt](https://img.shields.io/static/v1?style=for-the-badge&message=Nuxt&color=222222&logo=Nuxt&logoColor=00DC82&label=)
![Vue.js](https://img.shields.io/static/v1?style=for-the-badge&message=Vue.js&color=222222&logo=Vue.js&logoColor=4FC08D&label=)
![Tailwind CSS](https://img.shields.io/static/v1?style=for-the-badge&message=Tailwind+CSS&color=0B1120&logo=TailwindCSS&logoColor=38BDF8&label=)
![shadcn-vue](https://img.shields.io/static/v1?style=for-the-badge&message=shadcn-vue&color=000000&logo=shadcnui&logoColor=FFFFFF&label=)
![TypeScript](https://img.shields.io/static/v1?style=for-the-badge&message=TypeScript&color=3178C6&logo=TypeScript&logoColor=FFFFFF&label=)
![Prisma](https://img.shields.io/static/v1?style=for-the-badge&message=Prisma&color=2D3748&logo=Prisma&logoColor=FFFFFF&label=)
![PostgreSQL](https://img.shields.io/static/v1?style=for-the-badge&message=PostgreSQL&color=4169E1&logo=PostgreSQL&logoColor=FFFFFF&label=)
![Socket.io](https://img.shields.io/static/v1?style=for-the-badge&message=Socket.io&color=010101&logo=Socket.io&logoColor=FFFFFF&label=)
![Transformers.js](https://img.shields.io/static/v1?style=for-the-badge&message=Transformers.js&color=222222&logo=HuggingFace&logoColor=FFD21E&label=)
![Docker](https://img.shields.io/static/v1?style=for-the-badge&message=Docker&color=2496ED&logo=Docker&logoColor=FFFFFF&label=)
![Caddy](https://img.shields.io/static/v1?style=for-the-badge&message=Caddy&color=1F88C0&logo=Caddy&logoColor=FFFFFF&label=)

## ❤️ Contribuer

Les contributions sont les bienvenues. Lisez d'abord le [guide de contribution](CONTRIBUTING.md). [AGENTS.md](AGENTS.md) décrit l'architecture et les conventions du code.

## ⚖️ Licence

[MIT](LICENSE)
