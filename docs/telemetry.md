# Telemetry

Fileship collects anonymous usage data to understand how it is used and to guide development. Collection is **optional** and can be turned off at any time.

## What is collected

One report is sent about five minutes after startup and then every six hours. It contains counts, sizes, on/off values and version strings:

- **Instance:** a random ID and the Fileship version
- **System:** Node.js version, OS platform, release and architecture, CPU count and model, total memory, size and free space of the disk holding the data directory, whether Fileship runs in Docker, and uptime
- **Usage:** how many users, files (with sizes per category), folders, notes, short links and clicks, backups, inboxes and views in the last 30 days there are, and which storage types are configured. Items in the trash are counted separately.
- **Features:** whether registration, Turnstile, EXIF removal and AI are enabled, the trash retention, the log level, the AI preset and how many files were analyzed or marked as sensitive

No personal information, names, file contents, file names, URLs or domains are collected, and the telemetry server does not store IP addresses. See the [telemetry package](../packages/telemetry/README.md) for the full list of fields, and the [telemetry panel](../apps/telemetry/README.md) for how they are stored and counted.

## Opting out

Turn off **Anonymous telemetry** in **Administration → Instance**, or set `NUXT_TELEMETRY_ENABLED` to `false` in the `x-fileship` block of your `docker-compose.yml`.
