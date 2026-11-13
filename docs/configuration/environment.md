---
title: Environment variables
description: Every environment variable of the Fileship dashboard and the updater agent, with its default and the instance setting it seeds.
---

# Environment variables

Fileship reads these variables when it starts. With Docker, set them in the `x-fileship` block of `docker-compose.yml` (see [Setting variables with Docker Compose](../configuration.md#setting-variables-with-docker-compose)); without Docker, in `apps/dashboard/.env`. The full list with comments is in [`apps/dashboard/.env.example`](../../apps/dashboard/.env.example).

Variables marked **instance setting** only give the default of a field in **Administration → Instance**. Once a super admin saves that field, the saved value wins until it is reset with **Reset to default**. A value outside the field's allowed range falls back to the built-in default.

## Server

| Variable                 | Default          | Description                                                                                                                                                                                                                                              |
| ------------------------ | ---------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `DATABASE_URL`           | none, required   | PostgreSQL connection string, for example `postgres://user:password@host:5432/db`.                                                                                                                                                                       |
| `NUXT_DATA_DIRECTORY`    | `data`           | Uploads on the local storage, thumbnails, AI models, temporary files and `secret.key`. A relative path is resolved two folders above the folder Fileship starts in (`apps/dashboard`), which is the repository root; in Docker that is `/fileship/data`. |
| `HOST`                   | all addresses    | Address the server listens on. Compose sets `0.0.0.0`.                                                                                                                                                                                                   |
| `PORT`                   | `3000`           | Port the server listens on.                                                                                                                                                                                                                              |
| `NITRO_SHUTDOWN_TIMEOUT` | Nitro's default  | Milliseconds Fileship waits for running requests when it stops. Compose sets `25000`.                                                                                                                                                                    |
| `CHECKPOINT_DISABLE`     | set in the image | `1` stops the Prisma CLI from checking for updates when migrations run.                                                                                                                                                                                  |

## General

| Variable                   | Default    | Instance setting      | Description                                                                                       |
| -------------------------- | ---------- | --------------------- | ------------------------------------------------------------------------------------------------- |
| `NUXT_PUBLIC_SITE_NAME`    | `Fileship` | **Site name**         | Name shown in the browser title, link previews, emails and the dashboard.                         |
| `NUXT_SITE_URL`            | empty      | **Dashboard address** | Public address, such as `https://files.example.com`. Emails and single sign-on need it.           |
| `NUXT_PUBLIC_RETURN_HTTPS` | `auto`     | **Link protocol**     | `auto` (same as the request), `true` (always HTTPS) or `false` (always HTTP) for generated links. |

## Uploads, trash and backups

| Variable                         | Default | Instance setting           | Description                                             |
| -------------------------------- | ------- | -------------------------- | ------------------------------------------------------- |
| `NUXT_PUBLIC_FILE_CHUNK_SIZE`    | `25`    | **Chunk size (MB)**        | Size of upload chunks, 5 to 100 MB.                     |
| `NUXT_AVATAR_MAX_SIZE`           | `4`     | **Avatar size limit (MB)** | 1 to 100 MB.                                            |
| `NUXT_REMOVE_EXIF_DATA`          | `true`  | **Remove EXIF data**       | Strip location and camera details from uploaded images. |
| `NUXT_TRASH_RETENTION_DAYS`      | `30`    | **Trash retention (days)** | 0 to 365. `0` deletes immediately.                      |
| `NUXT_SCHEDULED_BACKUPS_ENABLED` | `true`  | **Scheduled backups**      | Whether users can back up their account on a schedule.  |

## Registration and single sign-on

| Variable                           | Default                              | Instance setting               | Description                                                                                     |
| ---------------------------------- | ------------------------------------ | ------------------------------ | ----------------------------------------------------------------------------------------------- |
| `NUXT_PUBLIC_REGISTRATION_ENABLED` | `false`                              | **Allow registration**         | Show a sign up link on the login page.                                                          |
| `NUXT_REGISTRATION_PERMISSIONS`    | `UploadFiles,TakeNotes,ShortenLinks` | **Permissions**                | Comma-separated. Only these three are honoured; admin rights are never granted.                 |
| `NUXT_REGISTRATION_USABLE_SPACE`   | `-1`                                 | **Storage limit (MB)**         | Storage limit of new accounts in MB, `-1` for unlimited.                                        |
| `NUXT_PUBLIC_TURNSTILE_SITE_KEY`   | empty                                | **Site key**                   | Cloudflare Turnstile. It only turns on when both keys are set.                                  |
| `NUXT_TURNSTILE_SECRET_KEY`        | empty                                | **Secret key**                 |                                                                                                 |
| `NUXT_OIDC_ENABLED`                | `false`                              | **Enable single sign-on**      | See [Single sign-on](single-sign-on.md).                                                        |
| `NUXT_OIDC_ISSUER_URL`             | empty                                | **Issuer URL**                 |                                                                                                 |
| `NUXT_OIDC_CLIENT_ID`              | empty                                | **Client ID**                  |                                                                                                 |
| `NUXT_OIDC_CLIENT_SECRET`          | empty                                | **Client secret**              |                                                                                                 |
| `NUXT_OIDC_BUTTON_LABEL`           | empty                                | **Provider name**              | Shown on the login button.                                                                      |
| `NUXT_OIDC_AUTO_REGISTER`          | `false`                              | **Create accounts**            | Create accounts for people who log in without one.                                              |
| `NUXT_PASSWORD_LOGIN_ENABLED`      | `true`                               | **Password and passkey login** | When `false` and single sign-on is set up, only super admins log in with a password or passkey. |

## Email

| Variable             | Default    | Instance setting   | Description                  |
| -------------------- | ---------- | ------------------ | ---------------------------- |
| `NUXT_SMTP_HOST`     | empty      | **SMTP server**    |                              |
| `NUXT_SMTP_PORT`     | `587`      | **Port**           |                              |
| `NUXT_SMTP_SECURITY` | `starttls` | **Encryption**     | `starttls`, `tls` or `none`. |
| `NUXT_SMTP_USER`     | empty      | **Username**       |                              |
| `NUXT_SMTP_PASSWORD` | empty      | **Password**       |                              |
| `NUXT_SMTP_FROM`     | empty      | **Sender address** |                              |

Email stays off until `NUXT_SITE_URL`, `NUXT_SMTP_HOST` and `NUXT_SMTP_FROM` are set. See [Email](email.md).

## AI

| Variable                    | Default  | Instance setting               | Description                             |
| --------------------------- | -------- | ------------------------------ | --------------------------------------- |
| `NUXT_PUBLIC_AI_ENABLED`    | `true`   | **AI features**                | Analyze uploads with the local models.  |
| `NUXT_AI_PRESET`            | `medium` | **Preset**                     | `low`, `medium` or `high`.              |
| `NUXT_AI_QUEUE_CONCURRENCY` | `1`      | **Parallel jobs**              | 1 to 8 files analyzed at the same time. |
| `NUXT_AI_INFERENCE_THREADS` | `2`      | **Threads per job**            | 1 to 64 CPU threads per job.            |
| `NUXT_AI_JOB_LEASE_MS`      | `120000` | **Job heartbeat timeout (ms)** | At least 60000.                         |

`NUXT_AI_SERVICE_URL` is the address of the [AI service](../ai.md#the-ai-service). The Docker image sets it to `http://fileship-ml:3003`. Left empty (the default without Docker), Fileship starts the AI service from `apps/ml/.venv` itself on `127.0.0.1:3003`. The AI service has no login of its own, so only point it at a service on a network you trust.

The AI service itself reads `FILESHIP_ML_HOST` (default `127.0.0.1`), `FILESHIP_ML_PORT` (`3003`) and `FILESHIP_ML_MODELS_DIR` (`data/models` of the repository). Its Docker image sets `0.0.0.0` and `/models`, and Fileship passes its own data directory when it starts the service.

## Logs and telemetry

| Variable                 | Default                                | Instance setting        | Description                                                              |
| ------------------------ | -------------------------------------- | ----------------------- | ------------------------------------------------------------------------ |
| `NUXT_LOG_LEVEL`         | `2`                                    | **Log level**           | `0` nothing, `1` important actions, `2` everything. See [Logs](logs.md). |
| `NUXT_TELEMETRY_ENABLED` | `true`                                 | **Anonymous telemetry** | See [Telemetry](../telemetry.md).                                        |
| `NUXT_TELEMETRY_URL`     | `https://fileship-telemetry.hanzy.dev` | none                    | Where reports go. Development builds only send reports when it is set.   |

## Updater

| Variable              | Default                     | Description                                                   |
| --------------------- | --------------------------- | ------------------------------------------------------------- |
| `NUXT_UPDATER_URL`    | `http://updater-agent:3001` | How the dashboard reaches the updater agent. Compose sets it. |
| `NUXT_UPDATER_SECRET` | `dev-secret-token`          | Shared secret. Compose sets it to `SECRET_TOKEN` from `.env`. |

## Build time

These are read when Fileship is built, so they only matter when you build it yourself ([without Docker](../install/without-docker.md), or with `docker-compose.dev.yml`). The published image uses the defaults.

| Variable              | Default        | Description                                                            |
| --------------------- | -------------- | ---------------------------------------------------------------------- |
| `ENABLE_PWA`          | `true`         | The web app manifest, the service worker and the Android share target. |
| `DELETE_EXPIRED_CRON` | `*/5 * * * *`  | When expired files, links and email tokens are deleted.                |
| `GENERATE_STATS_CRON` | `*/30 * * * *` | When the statistics are refreshed.                                     |

## Updater agent

The `updater-agent` service in `docker-compose.yml` reads its own variables:

| Variable              | Default              | Description                                                              |
| --------------------- | -------------------- | ------------------------------------------------------------------------ |
| `SECRET_TOKEN`        | none, required       | The secret from `.env`. The dashboard sends it with every request.       |
| `PORT`                | `3001`               | Port the agent listens on inside the Docker network.                     |
| `AUTOUPDATER_ENABLED` | off                  | `true` turns on scheduled updates. Compose sets `true`.                  |
| `AUTOUPDATER_CRON`    | `0 3 * * *`          | When scheduled updates run, in the agent's time zone.                    |
| `TZ`                  | UTC                  | Time zone for `AUTOUPDATER_CRON`, for example `Europe/Istanbul`.         |
| `COMPOSE_FILE`        | `docker-compose.yml` | The Compose file the agent runs, relative to the mounted project folder. |
| `FILESHIP_URL`        | `http://proxy:3000`  | Where the agent asks Fileship for its version.                           |
| `DEV_MODE`            | `false`              | Development mode; never turn it on for a real installation.              |

See [Updating](../updating.md).
