---
title: Configuration
description: How Fileship is configured - instance settings in the dashboard, environment variables for the rest, and where to set them with Docker Compose.
---

# Configuration

Fileship has two places for settings:

- **Instance settings**, edited by super admins in **Administration → Instance**. They apply immediately, without a restart, and are stored in the database. Most settings live here.
- **Environment variables**, read when Fileship starts. A few settings only exist here, because they are needed before the dashboard starts or are fixed when the app is built. For every instance setting, an environment variable gives the default.

## Instance settings

Super admins can configure Fileship from the dashboard in **Administration → Instance**:

- **General:** site name, dashboard address and link protocol
- **Uploads:** chunk size, avatar size limit, trash retention, EXIF removal and scheduled backups
- **Registration:** self-registration, default permissions (upload files, take notes, shorten links) and storage limit
- **Turnstile:** site and secret keys
- **Single sign-on:** an OpenID Connect provider, whether it creates accounts and whether passwords still work
- **Email:** SMTP server, encryption, credentials and sender address, plus **Send test email**
- **AI:** features on or off, preset, parallel jobs, threads per job and job heartbeat timeout, plus the analysis queue with **Retry failed** and **Reset AI data**
- **Logs and telemetry:** log level and anonymous telemetry
- **Webhooks:** whether [webhooks](webhooks.md) may reach private and local addresses (off by default)
- **Storage:** where uploads and backups are kept

[Instance settings](configuration/instance-settings.md) describes every field. Each of these settings except Webhooks and Storage can also be given a default through an environment variable. A value outside the allowed range falls back to the built-in default. A value saved in the dashboard always overrides the environment, and every field can be reset to its default.

## Topics

| Page                                                        | What it covers                                                         |
| ----------------------------------------------------------- | ---------------------------------------------------------------------- |
| [Environment variables](configuration/environment.md)       | Every variable, its default and the instance setting it seeds          |
| [Instance settings](configuration/instance-settings.md)     | Every field of **Administration → Instance**                           |
| [Storage](configuration/storage.md)                         | Local disk, S3 (AWS, Cloudflare R2, Backblaze B2, MinIO), SFTP and FTP |
| [Email](configuration/email.md)                             | SMTP, confirmation and reset links, invites and storage warnings       |
| [Single sign-on](configuration/single-sign-on.md)           | OpenID Connect providers                                               |
| [Registration and invites](configuration/registration.md)   | Self-registration, invites and Turnstile                               |
| [Trash and retention](configuration/trash-and-retention.md) | Trash, expiry, view limits and scheduled backups                       |
| [Logs](configuration/logs.md)                               | Log level, the Logs page and server output                             |

## Storage

Uploads and backups are kept in the data directory by default, or on S3-compatible storage, SFTP or FTP. See [Storage](configuration/storage.md).

## Email

Fileship can send confirmation and password reset links, invites and storage warnings. See [Email](configuration/email.md).

## Single sign-on

Fileship can log people in through an OpenID Connect provider such as Authentik, Authelia, Keycloak, Pocket ID, Zitadel or Google. See [Single sign-on](configuration/single-sign-on.md).

## Trash

Deleted files, folders and notes move to the trash for 30 days by default. See [Trash and retention](configuration/trash-and-retention.md).

## Scheduled backups

Users can back up their account automatically every day or every week. See [Backups](features/backups.md) and [Trash and retention](configuration/trash-and-retention.md#scheduled-backups).

## Environment-only settings

A few settings can only be set through the environment:

| Variable                                      | Description                                                                     |
| --------------------------------------------- | ------------------------------------------------------------------------------- |
| `DATABASE_URL`                                | PostgreSQL connection string (required)                                         |
| `NUXT_DATA_DIRECTORY`                         | Local storage, thumbnails and temporary files (default: `data`)                 |
| `NUXT_UPDATER_URL` / `NUXT_UPDATER_SECRET`    | How the dashboard reaches the updater agent                                     |
| `NUXT_TELEMETRY_URL`                          | Where anonymous telemetry reports are sent                                      |
| `DELETE_EXPIRED_CRON` / `GENERATE_STATS_CRON` | Schedules for removing expired files and links, and for statistics (build time) |
| `ENABLE_PWA`                                  | Installable app and the Android share target (build time)                       |

Build time options only take effect when you build Fileship yourself, for example with `docker-compose.dev.yml` or [without Docker](install/without-docker.md). The published image always uses their defaults. [Environment variables](configuration/environment.md) lists every variable.

### Setting variables with Docker Compose

Add the variable to the shared `x-fileship` block in `docker-compose.yml`, so both the `fileship-blue` and `fileship-green` containers receive it:

```yaml
x-fileship: &fileship
    environment:
        NUXT_PUBLIC_REGISTRATION_ENABLED: 'true'
```

Then apply it with `docker compose up -d`. Keep the variables that are already in the block (`HOST`, `PORT`, `DATABASE_URL`, the updater ones and `NITRO_SHUTDOWN_TIMEOUT`).

`docker-compose.yml` is part of the repository, so `git pull` refuses to update it while it has your changes. Commit them first (`git commit -am "Local settings"`), and `git pull` then merges the new version with yours.

### Setting variables without Docker

Put them in `apps/dashboard/.env` (or your service's environment) and restart Fileship. See [Install without Docker](install/without-docker.md#4-configure-it).
