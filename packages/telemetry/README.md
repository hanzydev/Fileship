# Fileship Telemetry Module

Fileship collects telemetry data about general usage. This helps us to accurately gauge Fileship feature usage and customization across all our users.

This program is optional. You can always [opt-out](#opting-out) if you do not want to share any information.

## ⚙️ Data Collected

The telemetry module collects the following data points:

### ℹ️ Fileship Information

- `projectId`: A unique, anonymous identifier for the Fileship instance.
- `version`: The version of the Fileship application.
- `files`: The total count of files uploaded.
- `folders`: The total count of folders created.
- `notes`: The total count of notes stored.
- `users`: The total count of registered users.
- `nodeVersion`: The Node.js version running Fileship.
- `createdAt`: Timestamp when this specific telemetry record was created.

### 💻 System Information

- `projectId`: A unique, anonymous identifier for the Fileship instance.
- `cpuCount`: Number of CPU cores.
- `cpuModel`: Model name of the CPU.
- `memoryTotal`: Total system memory (RAM) in bytes.
- `memoryFree`: Free system memory (RAM) in bytes.
- `memoryUsed`: Used system memory (RAM) in bytes.
- `diskTotal`: Total disk space in bytes.
- `diskFree`: Free disk space in bytes.
- `diskUsed`: Used disk space in bytes.
- `osPlatform`: Operating system platform (e.g., 'linux', 'darwin').
- `osRelease`: Operating system release version.
- `osArch`: Operating system architecture (e.g., 'x64').
- `isDocker`: Boolean indicating if Fileship is running inside a Docker container.
- `createdAt`: Timestamp when this specific telemetry record was created.

**Important Note:** `projectId` is an anonymous identifier for your Fileship instance and cannot be traced back to individual users. Fileship aims to collect minimal data necessary for product improvement.

## 🔔 Opting-out

Users can opt-out of telemetry data collection by setting the `TELEMETRY_ENABLED` environment variable to `false`. This will disable all telemetry data collection.

## ⚖️ License

[MIT](https://github.com/hanzydev/Fileship/blob/main/LICENSE)
