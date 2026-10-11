# @fileship/telemetry

Anonymous usage telemetry for Fileship. The dashboard uses this internal package to send periodic usage statistics, which help guide development and prioritize features.

Telemetry is optional and can be [turned off](#opting-out) at any time.

## ⚙️ Data collected

Telemetry is only sent from production builds. It contains no personal information, file contents, file names or user names.

### ℹ️ Instance information

| Field         | Description                             |
| ------------- | --------------------------------------- |
| `projectId`   | A random, anonymous ID for the instance |
| `version`     | The Fileship version                    |
| `files`       | Total number of files                   |
| `folders`     | Total number of folders                 |
| `notes`       | Total number of notes                   |
| `users`       | Total number of users                   |
| `nodeVersion` | The Node.js version running Fileship    |
| `createdAt`   | When the record was created             |

### 💻 System information

| Field         | Description                                     |
| ------------- | ----------------------------------------------- |
| `projectId`   | A random, anonymous ID for the instance         |
| `cpuCount`    | Number of CPU cores                             |
| `cpuModel`    | CPU model name                                  |
| `memoryTotal` | Total memory, in bytes                          |
| `memoryFree`  | Free memory, in bytes                           |
| `memoryUsed`  | Used memory, in bytes                           |
| `diskTotal`   | Total disk space, in bytes                      |
| `diskFree`    | Free disk space, in bytes                       |
| `diskUsed`    | Used disk space, in bytes                       |
| `osPlatform`  | Operating system platform (for example `linux`) |
| `osRelease`   | Operating system release                        |
| `osArch`      | CPU architecture (for example `x64`)            |
| `isDocker`    | Whether Fileship runs inside a Docker container |
| `createdAt`   | When the record was created                     |

The `projectId` is generated randomly on first start and stored in the data directory. It cannot be traced back to a person or a server.

## 🔔 Opting out

Super admins can turn off **Anonymous telemetry** in **Administration → Instance**. Alternatively, set the `NUXT_TELEMETRY_ENABLED` environment variable to `false`.

## ⚖️ License

[MIT](https://github.com/hanzydev/Fileship/blob/main/LICENSE)
