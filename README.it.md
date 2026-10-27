<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    Il tuo server ShareX e il tuo cloud per i file: un'app Windows nativa, la sincronizzazione con Esplora file, una ricerca con IA che gira sul tuo server e un'estensione per il browser.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Scarica per Windows</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Segnala un bug</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Richiedi una funzionalità</a>
    ·
    <a href="SECURITY.md">Politica di sicurezza</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <b>Italiano</b> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Un'immagine trascinata su Fileship viene caricata e il suo link copiato, poi la ricerca intelligente trova una foto a partire da una descrizione e ne mostra la descrizione generata dall'IA" width="100%">
</p>

## 📦 Funzionalità

- File, cartelle annidate da condividere con altri utenti, note in Markdown, link brevi e caselle di ricezione, con un cestino per tutto ciò che elimini
- Backup dell'account da scaricare e ripristinare, creati a mano o in base a una pianificazione
- Condivisione pubblica con password, data di scadenza, limite di visualizzazioni e anteprime dei link curate per Discord e altre app
- [IA](docs/ai.md) che gira sul tuo server: ricerca intelligente in oltre 50 lingue, descrizioni, OCR e rilevamento dei contenuti sensibili, tutto facoltativo
- Caricamenti sul disco locale, su qualsiasi servizio compatibile con S3, su SFTP o FTP
- Più utenti con autorizzazioni e limiti di spazio, passkey, autenticazione a due fattori e [Single Sign-On](docs/configuration.md#single-sign-on) con OpenID Connect
- Aggiornamenti senza interruzioni del servizio, con un clic o pianificati
- Integrazioni di caricamento per ShareX, macOS e Linux, l'[app per Windows](docs/desktop.md) e un'[app installabile per smartphone](docs/mobile.md) con cui le app Android possono condividere
- Un'[estensione per il browser](docs/browser-extension.md) che carica file, immagini e screenshot e accorcia i link, [sul Chrome Web Store](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh) per Chrome ed Edge, e [sui componenti aggiuntivi di Firefox](https://addons.mozilla.org/firefox/addon/fileship/) per Firefox
- [Webhook](docs/webhooks.md) per Discord, Slack e i tuoi servizi
- Inglese, turco, tedesco, spagnolo, francese, italiano, portoghese brasiliano, russo, azerbaigiano, giapponese, coreano e cinese semplificato

## 🆚 Fileship a confronto

|                                                        | Fileship                         | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| ------------------------------------------------------ | -------------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX e altri strumenti di caricamento                | ✅                               | ✅                                          | ✅                                                 | ✅                                                  |
| App desktop per acquisire e registrare lo schermo      | ✅ Windows                       | ❌                                          | ❌                                                 | ❌                                                  |
| I tuoi file in Esplora file                            | ✅ Fileship Harbor               | ❌                                          | ❌                                                 | ❌                                                  |
| Estensione per il browser                              | ✅                               | ❌                                          | ❌                                                 | ✅                                                  |
| Ricerca con IA, descrizioni, OCR e contenuti sensibili | ✅ sul tuo server                | ❌                                          | ❌                                                 | ❌                                                  |
| Link brevi                                             | ✅                               | ✅                                          | ❌                                                 | ✅                                                  |
| Note o paste                                           | ✅ Markdown                      | ✅                                          | 🟡 caricamento di testo                            | ✅                                                  |
| Cartelle                                               | ✅ annidate, condivise con altri | ✅ annidate                                 | ❌ tag                                             | 🟡 album su un solo livello                         |
| Password, scadenza e limiti di visualizzazioni         | ✅                               | ✅                                          | ❌                                                 | 🟡 link degli album con scadenza                    |
| Archiviazione                                          | Locale, S3, SFTP, FTP            | Locale, S3                                  | Locale, S3, FTP, Dropbox, Google Cloud             | Locale, S3                                          |
| Autenticazione a due fattori e passkey                 | ✅                               | ✅                                          | ❌                                                 | ❌                                                  |
| Single Sign-On                                         | ✅ OIDC                          | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhook                                                | ✅ Discord, Slack, JSON firmato  | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| Backup dell'account ripristinabili e pianificati       | ✅                               | 🟡 esportazione                             | 🟡 esportazione                                    | ❌                                                  |
| Aggiornamenti dalla dashboard                          | ✅ senza interruzioni            | ❌                                          | 🟡 con interruzione                                | ❌                                                  |
| Lingue                                                 | 12                               | 12                                          | 31, alcune parziali                                | 1                                                   |
| Licenza                                                | MIT                              | MIT                                         | AGPL-3.0                                           | MIT                                                 |

Confronto con Zipline 4.8, XBackBone 3.8 e Chibisafe 6.7 a ottobre 2026. Qualcosa non è aggiornato? [Apri una issue](https://github.com/hanzydev/Fileship/issues/new/choose) e verrà corretto.

## 📸 Screenshot

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Dashboard di Fileship su computer" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="File di Fileship su smartphone" width="18%">
</p>

## 🖥️ Fileship per Windows

Acquisisci, registra e carica sul tuo Fileship dall'area di notifica, come con ShareX, e sfoglia i tuoi file in Esplora file con Fileship Harbor. Richiede Windows 11 a 64 bit versione 24H2 o successiva e un server con Fileship 6.0.

**[Scarica l'ultima versione](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [Funzionalità e configurazione](docs/desktop.md)

## 🚀 Per iniziare

Ti servono [Docker](https://www.docker.com/) con [Docker Compose](https://docs.docker.com/compose/), 2 core di CPU e 2 GB di RAM (4 GB o più con l'IA, vedi i [requisiti](docs/installation.md#requirements)).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

Fileship è ora disponibile su `http://localhost:3000`. Accedi con `admin` / `password`.

> [!IMPORTANT]
> Cambia la password subito dopo il primo accesso (**Impostazioni → Sicurezza**).

Arrivi dalla 5.x? Segui [Aggiornare alla 6.0](docs/upgrading.md). Arrivi da Zipline, XBackBone o Chibisafe? [Porta con te account, file e link](docs/migrating.md).

## 📚 Documentazione

- [Installazione](docs/installation.md): requisiti, browser supportati e Fileship sul tuo dominio
- [Configurazione](docs/configuration.md): impostazioni del server, archiviazione, cestino e variabili d'ambiente
- [Aggiornamenti](docs/updating.md): come funzionano gli aggiornamenti e come pianificarli
- [Passare da un'altra app](docs/migrating.md): importare un server Zipline, XBackBone o Chibisafe
- [Funzionalità di IA](docs/ai.md): preset, modelli, ricerca intelligente e contenuti sensibili
- [Fileship per Windows](docs/desktop.md)
- [Fileship sullo smartphone](docs/mobile.md): installare l'app e condividere da Android
- [Webhook](docs/webhooks.md): eventi, formati, consegna e verifica della firma
- [Telemetria](docs/telemetry.md): cosa viene raccolto e come disattivarla

## 🧰 Realizzato con

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

## ❤️ Contribuire

I contributi sono benvenuti. Leggi prima le [linee guida per contribuire](CONTRIBUTING.md). [AGENTS.md](AGENTS.md) descrive l'architettura e le convenzioni del codice.

## ⚖️ Licenza

[MIT](LICENSE)
