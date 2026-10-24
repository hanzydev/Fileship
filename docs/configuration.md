# Configuration

## Instance settings

Super admins can configure Fileship from the dashboard in **Administration → Instance**. The changes apply immediately, without a restart:

- **General:** site name and link protocol
- **Uploads:** chunk size, avatar size limit, trash retention, EXIF removal and scheduled backups
- **Registration:** self-registration, default permissions (upload files, take notes, shorten links) and storage limit
- **Turnstile:** site and secret keys
- **AI:** features on or off, preset, parallel jobs, threads per job and job heartbeat timeout, plus the analysis queue with **Retry failed** and **Reset AI data**
- **Logs and telemetry:** log level and anonymous telemetry
- **Storage:** where uploads and backups are kept

Each of these settings except Storage can also be given a default through an environment variable (see [`apps/dashboard/.env.example`](../apps/dashboard/.env.example)). A value outside the allowed range falls back to the built-in default. A value saved in the dashboard always overrides the environment, and every field can be reset to its default.

## Storage

Fileship keeps uploads and backups in its data directory by default. In **Administration → Instance → Storage** you can add S3 compatible storage (AWS S3, Cloudflare R2, MinIO, Backblaze B2 and others), SFTP or FTP, and choose which one new files go to. Each file stays where it was written, so switching storage does not move anything. Use **Move everything here** on the active storage to move existing files and backups to it.

Uploads stream to the storage chunk by chunk. Thumbnails and temporary files always stay in the data directory. Storage credentials are encrypted with `data/secret.key`, so keep that file together with your data.

On every start, Fileship removes files and backups whose data is missing from their storage. A storage that cannot be reached or looks empty is skipped, so a misconfigured storage never deletes anything.

## Trash

Deleted files, folders and notes move to the trash for 30 days by default. Set **Trash retention** in **Administration → Instance** (or `NUXT_TRASH_RETENTION_DAYS`) to change it; `0` deletes everything immediately. Items in the trash still count toward a user's storage limit until they are deleted for good. Expired files and files that reach their view limit are always deleted immediately.

## Scheduled backups

Users can back up their account automatically in **Settings → Backups**: every day or every week, at an hour in their own time zone, keeping the newest automatic backups up to the number they choose. Backups they create or upload themselves are never deleted automatically. A user's backup limit counts both kinds, so a scheduled backup is skipped while manual backups alone fill it. Due backups are checked every 15 minutes, and an instance that was offline makes up a missed backup once when it starts again.

Turn **Scheduled backups** off in **Administration → Instance** (or set `NUXT_SCHEDULED_BACKUPS_ENABLED=false`) to stop them for everyone. Users' schedules are kept and continue when the setting is turned back on.

## Environment-only settings

A few settings can only be set through the environment, because they are needed before the dashboard starts or are fixed when the app is built:

| Variable                                      | Description                                                                     |
| --------------------------------------------- | ------------------------------------------------------------------------------- |
| `DATABASE_URL`                                | PostgreSQL connection string (required)                                         |
| `NUXT_DATA_DIRECTORY`                         | Local storage, thumbnails and temporary files (default: `data`)                 |
| `NUXT_UPDATER_URL` / `NUXT_UPDATER_SECRET`    | How the dashboard reaches the updater agent                                     |
| `NUXT_TELEMETRY_URL`                          | Where anonymous telemetry reports are sent                                      |
| `DELETE_EXPIRED_CRON` / `GENERATE_STATS_CRON` | Schedules for removing expired files and links, and for statistics (build time) |
| `ENABLE_PWA`                                  | Progressive Web App support (build time)                                        |

Build time options only take effect when you build the image yourself, for example with `docker-compose.dev.yml`. The published image always uses their defaults.

To set an environment variable with Docker Compose, add it to the shared `x-fileship` block in `docker-compose.yml`, so both the `fileship-blue` and `fileship-green` containers receive it:

```yaml
x-fileship: &fileship
    environment:
        NUXT_PUBLIC_REGISTRATION_ENABLED: 'true'
```

Then apply it with `docker compose up -d`.
