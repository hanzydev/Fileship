# @fileship/telemetry

Anonymous usage telemetry for Fileship. The dashboard uses this internal package to send a usage report, which helps guide development and prioritize features. The package also exports the report schema, so the telemetry panel validates exactly what the dashboard sends.

Telemetry is optional and can be [turned off](#opting-out) at any time.

## ⚙️ Data collected

The dashboard sends one report about five minutes after startup and then every six hours, to `POST /api/v2/report`. Development builds only send it when `NUXT_TELEMETRY_URL` is set. Nothing is collected while telemetry is turned off.

The report contains counts, sizes, on/off values and version strings. It never contains personal information, names, file contents, file names, captions, URLs, domains or setting values such as keys. The telemetry server does not store IP addresses.

### ℹ️ Instance

| Field     | Description                                                           |
| --------- | --------------------------------------------------------------------- |
| `id`      | A random ID created with the first report, kept in the data directory |
| `version` | The Fileship version                                                  |
| `sentAt`  | When the report was sent                                              |

### 💻 Runtime

| Field         | Description                                           |
| ------------- | ----------------------------------------------------- |
| `node`        | The Node.js version                                   |
| `platform`    | Operating system platform (for example `linux`)       |
| `release`     | Operating system release                              |
| `arch`        | CPU architecture (for example `x64`)                  |
| `cpuCount`    | Number of CPU cores                                   |
| `cpuModel`    | CPU model name                                        |
| `memoryTotal` | Total memory, in bytes                                |
| `diskTotal`   | Size of the disk holding the data directory, in bytes |
| `diskFree`    | Free space on that disk, in bytes                     |
| `docker`      | Whether Fileship runs inside a Docker container       |
| `uptime`      | Seconds since Fileship started                        |

### 📁 Usage

Totals leave out items in the trash, which are counted in `trashed`.

| Field              | Description                                                                           |
| ------------------ | ------------------------------------------------------------------------------------- |
| `users`            | Users, admins, users with an authenticator app, users with passkeys and upload tokens |
| `files`            | Files, trashed files, total size, and the count and size per category                 |
| `folders`, `notes` | Total, public and trashed                                                             |
| `links`            | Short links, their total clicks and how many are password protected                   |
| `backups`          | Backups and their total size                                                          |
| `inboxes`          | Inboxes and how many are enabled                                                      |
| `views`            | File views in the last 30 days                                                        |
| `storages`         | The active storage type and how many storages of each type exist                      |

File categories are `image`, `video`, `audio`, `document`, `archive`, `code` and `other`.

### 🧩 Features

| Field                | Description                                                                              |
| -------------------- | ---------------------------------------------------------------------------------------- |
| `registration`       | Whether self-registration is enabled                                                     |
| `trashRetentionDays` | Days items stay in the trash                                                             |
| `logLevel`           | The log level                                                                            |
| `turnstile`          | Whether Turnstile is configured (the keys are never sent)                                |
| `removeExifData`     | Whether EXIF data is removed from uploads                                                |
| `ai`                 | Whether AI is enabled, the preset, analyzed and sensitive files, pending and failed jobs |

## 🔔 Opting out

Super admins can turn off **Anonymous telemetry** in **Administration → Instance**. Alternatively, set the `NUXT_TELEMETRY_ENABLED` environment variable to `false`.

## ⚖️ License

[MIT](https://github.com/hanzydev/Fileship/blob/main/LICENSE)
