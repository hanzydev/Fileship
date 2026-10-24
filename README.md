<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    An open-source, self-hosted file and media management platform.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Download for Windows</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Report a bug</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Request a feature</a>
    ·
    <a href="SECURITY.md">Security policy</a>
  </p>
</div>

## 📸 Screenshots

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Fileship dashboard on desktop" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Fileship files on mobile" width="18%">
</p>

## 📦 Features

- Files, folders, Markdown notes, short links and inboxes, with a trash for anything you delete
- Public sharing with passwords, expiry dates, view limits and rich embeds for Discord and other apps
- [AI](docs/ai.md) that runs on your server: smart search in over 50 languages, captions, OCR and sensitive content detection, all optional
- Uploads on the local disk, any S3 compatible service, SFTP or FTP
- Multiple users with permissions and storage limits, passkeys and two-factor authentication
- Zero-downtime updates, one click or on a schedule
- ShareX, macOS and Linux upload integrations, and the [Windows app](docs/desktop.md)
- [Webhooks](docs/webhooks.md) for Discord, Slack and your own services
- English, Turkish, German, French, Italian, Azerbaijani, Japanese, Korean and Simplified Chinese

## 🖥️ Fileship for Windows

Capture, record and upload to your Fileship from the system tray, ShareX style, and browse your files in File Explorer with Fileship Harbor. Needs Windows 10 or 11 (64-bit) and a Fileship 6.0 server.

**[Download the latest version](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [Features and setup](docs/desktop.md)

## 🚀 Getting Started

You need [Docker](https://www.docker.com/) with [Docker Compose](https://docs.docker.com/compose/), 2 CPU cores and 2 GB of RAM (4 GB or more with AI, see the [requirements](docs/installation.md#requirements)).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

Fileship is now available at `http://localhost:3000`. Log in with `admin` / `password`.

> [!IMPORTANT]
> Change the password right after your first login (**Settings → Security**).

Upgrading from 5.x? Follow [Upgrading to 6.0](docs/upgrading.md).

## 📚 Documentation

- [Installation](docs/installation.md): requirements, browser support and serving Fileship on your own domain
- [Configuration](docs/configuration.md): instance settings, storage, trash and environment variables
- [Updating](docs/updating.md): how updates work and how to schedule them
- [AI features](docs/ai.md): presets, models, smart search and sensitive content
- [Fileship for Windows](docs/desktop.md)
- [Webhooks](docs/webhooks.md): events, formats, delivery and signature verification
- [Telemetry](docs/telemetry.md): what is collected and how to turn it off

## 🧰 Built With

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

## ❤️ Contributing

Contributions are welcome. Please read the [contribution guidelines](CONTRIBUTING.md) first. [AGENTS.md](AGENTS.md) describes the architecture and conventions of the codebase.

## ⚖️ License

[MIT](LICENSE)
