<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    Dein eigener ShareX-Server und deine eigene Datei-Cloud: eine native Windows-App, Synchronisierung mit dem Explorer, KI-Suche, die auf deinem Server läuft, und eine Browser-Erweiterung.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Für Windows herunterladen</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Fehler melden</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Funktion vorschlagen</a>
    ·
    <a href="SECURITY.md">Sicherheitsrichtlinie</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <b>Deutsch</b> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Ein auf Fileship gezogenes Bild wird hochgeladen und sein Link kopiert, dann findet die intelligente Suche ein Foto anhand einer Beschreibung und zeigt seine KI-Bildbeschreibung" width="100%">
</p>

## 📦 Funktionen

- Dateien, verschachtelte Ordner, die du mit anderen Benutzern teilen kannst, Markdown-Notizen, Kurzlinks und Posteingänge, dazu ein Papierkorb für alles, was du löschst
- Konto-Backups zum Herunterladen und Wiederherstellen, von Hand oder nach Zeitplan erstellt
- Öffentliche Freigaben mit Passwort, Ablaufdatum, Aufruflimit und ansprechender Link-Vorschau für Discord und andere Apps
- [KI](docs/ai.md), die auf deinem Server läuft: intelligente Suche in über 50 Sprachen, Bildbeschreibungen, OCR und Erkennung sensibler Inhalte, alles optional
- Uploads auf die lokale Festplatte, jeden S3-kompatiblen Dienst, SFTP oder FTP
- Mehrere Benutzer mit Berechtigungen und Speicherlimits, Passkeys, Zwei-Faktor-Authentifizierung und [Single Sign-On](docs/configuration.md#single-sign-on) mit OpenID Connect
- Updates ohne Ausfallzeit, per Klick oder nach Zeitplan
- Upload-Integrationen für ShareX, macOS und Linux, die [Windows-App](docs/desktop.md) und eine [installierbare App fürs Smartphone](docs/mobile.md), an die Android-Apps Inhalte teilen können
- Eine [Browser-Erweiterung](docs/browser-extension.md), die Dateien, Bilder und Screenshots hochlädt und Links kürzt, [im Chrome Web Store](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh) für Chrome und Edge, für Firefox [bei Firefox Add-ons](https://addons.mozilla.org/firefox/addon/fileship/)
- [Webhooks](docs/webhooks.md) für Discord, Slack und deine eigenen Dienste
- Englisch, Türkisch, Deutsch, Spanisch, Französisch, Italienisch, brasilianisches Portugiesisch, Russisch, Aserbaidschanisch, Japanisch, Koreanisch und vereinfachtes Chinesisch

## 🆚 Fileship im Vergleich

|                                                     | Fileship                              | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| --------------------------------------------------- | ------------------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX und andere Uploader                          | ✅                                    | ✅                                          | ✅                                                 | ✅                                                  |
| Desktop-App für Screenshots und Aufnahmen           | ✅ Windows                            | ❌                                          | ❌                                                 | ❌                                                  |
| Deine Dateien im Explorer                           | ✅ Fileship Harbor                    | ❌                                          | ❌                                                 | ❌                                                  |
| Browser-Erweiterung                                 | ✅                                    | ❌                                          | ❌                                                 | ✅                                                  |
| KI-Suche, Bildbeschreibungen, OCR, sensible Inhalte | ✅ auf deinem Server                  | ❌                                          | ❌                                                 | ❌                                                  |
| Kurzlinks                                           | ✅                                    | ✅                                          | ❌                                                 | ✅                                                  |
| Notizen oder Pastes                                 | ✅ Markdown                           | ✅                                          | 🟡 Text-Uploads                                    | ✅                                                  |
| Ordner                                              | ✅ verschachtelt, mit anderen geteilt | ✅ verschachtelt                            | ❌ Tags                                            | 🟡 flache Alben                                     |
| Passwörter, Ablaufdatum und Aufruflimits            | ✅                                    | ✅                                          | ❌                                                 | 🟡 Album-Links laufen ab                            |
| Speicher                                            | Lokal, S3, SFTP, FTP                  | Lokal, S3                                   | Lokal, S3, FTP, Dropbox, Google Cloud              | Lokal, S3                                           |
| Zwei-Faktor-Authentifizierung und Passkeys          | ✅                                    | ✅                                          | ❌                                                 | ❌                                                  |
| Single Sign-On                                      | ✅ OIDC                               | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhooks                                            | ✅ Discord, Slack, signiertes JSON    | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| Wiederherstellbare Konto-Backups nach Zeitplan      | ✅                                    | 🟡 Export                                   | 🟡 Export                                          | ❌                                                  |
| Updates aus dem Dashboard                           | ✅ ohne Ausfallzeit                   | ❌                                          | 🟡 mit Ausfallzeit                                 | ❌                                                  |
| Sprachen                                            | 12                                    | 12                                          | 31, teils unvollständig                            | 1                                                   |
| Lizenz                                              | MIT                                   | MIT                                         | AGPL-3.0                                           | MIT                                                 |

Verglichen mit Zipline 4.8, XBackBone 3.8 und Chibisafe 6.7 im Oktober 2026. Etwas ist veraltet? [Erstelle ein Issue](https://github.com/hanzydev/Fileship/issues/new/choose), dann wird es korrigiert.

## 📸 Screenshots

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Fileship-Dashboard am Computer" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Fileship-Dateien auf dem Smartphone" width="18%">
</p>

## 🖥️ Fileship für Windows

Nimm Screenshots und Videos auf und lade sie wie mit ShareX direkt aus dem Infobereich auf dein Fileship hoch, und durchsuche deine Dateien mit Fileship Harbor im Explorer. Erfordert Windows 11 (64 Bit) ab Version 24H2 und einen Server mit Fileship 6.0.

**[Neueste Version herunterladen](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [Funktionen und Einrichtung](docs/desktop.md)

## 🚀 Erste Schritte

Du brauchst [Docker](https://www.docker.com/) mit [Docker Compose](https://docs.docker.com/compose/), 2 CPU-Kerne und 2 GB RAM (4 GB oder mehr mit KI, siehe [Systemanforderungen](docs/installation.md#requirements)).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

Fileship ist jetzt unter `http://localhost:3000` erreichbar. Melde dich mit `admin` / `password` an.

> [!IMPORTANT]
> Ändere das Passwort gleich nach der ersten Anmeldung (**Einstellungen → Sicherheit**).

Du kommst von 5.x? Folge der Anleitung [Upgrade auf 6.0](docs/upgrading.md). Du kommst von Zipline, XBackBone oder Chibisafe? [Übernimm deine Konten, Dateien und Links](docs/migrating.md).

## 📚 Dokumentation

- [Installation](docs/installation.md): Voraussetzungen, unterstützte Browser und Fileship unter deiner eigenen Domain
- [Konfiguration](docs/configuration.md): Servereinstellungen, Speicher, Papierkorb und Umgebungsvariablen
- [Updates](docs/updating.md): wie Updates funktionieren und wie du sie planst
- [Umzug von einer anderen App](docs/migrating.md): einen Zipline-, XBackBone- oder Chibisafe-Server importieren
- [KI-Funktionen](docs/ai.md): Voreinstellungen, Modelle, intelligente Suche und sensible Inhalte
- [Fileship für Windows](docs/desktop.md)
- [Fileship auf dem Smartphone](docs/mobile.md): die App installieren und von Android aus teilen
- [Webhooks](docs/webhooks.md): Ereignisse, Formate, Zustellung und Prüfung der Signatur
- [Telemetrie](docs/telemetry.md): was erfasst wird und wie du es abschaltest

## 🧰 Entwickelt mit

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

## ❤️ Mitwirken

Beiträge sind willkommen. Lies bitte zuerst die [Richtlinien für Beiträge](CONTRIBUTING.md). [AGENTS.md](AGENTS.md) beschreibt die Architektur und die Konventionen des Codes.

## ⚖️ Lizenz

[MIT](LICENSE)
