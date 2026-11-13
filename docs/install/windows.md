---
title: Windows
description: Run a Fileship server on Windows, with Docker Desktop on WSL 2 or without Docker, with every command for PowerShell.
---

# Windows

Fileship runs on Windows in two ways:

- [With Docker](#with-docker): Docker Desktop on WSL 2 runs the same stack as Linux, with one-click updates. Recommended.
- [Without Docker](#without-docker): Node.js and PostgreSQL installed on Windows, run as a service.

Looking for the app that captures and uploads screenshots? That is [Fileship for Windows](../desktop.md); this page is about running the server.

Commands are for PowerShell. Run the ones marked **as administrator** in a PowerShell window opened with **Run as administrator**.

## With Docker

### 1. Install WSL 2 and Docker Desktop

As administrator:

```powershell
wsl --install
```

Restart Windows when it asks. Then install Docker Desktop and Git:

```powershell
winget install -e --id Docker.DockerDesktop
winget install -e --id Git.Git
```

Start Docker Desktop and finish its setup. It uses the WSL 2 engine by default (**Settings → General → Use the WSL 2 based engine**). Check that it works in a new PowerShell window:

```powershell
docker version
docker compose version
```

### 2. Clone Fileship

Pick a folder for Fileship. Its `data` folder will hold your files, so choose a drive with enough space:

```powershell
cd C:\
git clone https://github.com/hanzydev/Fileship.git
cd C:\Fileship
```

Git for Windows may convert line endings to CRLF when it checks out files. Docker and Fileship read both, so this is harmless.

### 3. Generate the updater secret

Windows has no `openssl` command, so generate the secret with PowerShell. This works in Windows PowerShell 5.1 and PowerShell 7:

```powershell
$bytes = New-Object byte[] 32
[Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bytes)
$hex = -join ($bytes | ForEach-Object { $_.ToString('x2') })
"SECRET_TOKEN=$hex" | Set-Content -Encoding ascii .env
```

`-Encoding ascii` matters: Windows PowerShell writes UTF-16 by default, which Docker Compose cannot read.

### 4. Start Fileship

```powershell
docker compose up -d
```

Open `http://localhost:3000` and log in with `admin` / `password`, then follow [First steps](first-steps.md). Your files are in `C:\Fileship\data`, the database in the `pg_data` Docker volume.

### Reach it from other devices

Docker Desktop forwards port 3000 to Windows. To open it to your network, allow it in the firewall (as administrator):

```powershell
New-NetFirewallRule -DisplayName "Fileship" -Direction Inbound -Protocol TCP -LocalPort 3000 -Action Allow
```

For a domain and HTTPS, run a reverse proxy on Windows that sends requests to `127.0.0.1:3000`: [Caddy for Windows](../proxy/caddy.md#windows) or [IIS](../proxy/iis.md).

### Start with Windows

Containers start again whenever Docker Desktop starts (`restart: unless-stopped`). In Docker Desktop, turn on **Settings → General → Start Docker Desktop when you sign in to your computer**.

> [!NOTE]
> Docker Desktop only runs while a user is signed in. For a server that must come back after a reboot without anyone signing in, configure automatic sign-in for a dedicated account, or use [Fileship without Docker](#without-docker) as a Windows service.

### Updates

One-click and scheduled updates work the same as on Linux (see [Updating](../updating.md)). The updater agent finds the Compose project through Docker Desktop's path for Windows drives (`/run/desktop/mnt/host/<drive>/…`), so the clone can live on any drive.

## Without Docker

You build Fileship from source and run it with Node.js, like the [Linux guide](without-docker.md). You give up the same things: one-click updates, updates without downtime, automatic PostgreSQL upgrades and the built-in proxy.

### 1. Install Node.js, Git and PostgreSQL

Fileship needs Node.js 24. As administrator:

```powershell
winget install -e --id OpenJS.NodeJS.LTS --version 24.22.1
winget install -e --id Git.Git
winget install -e --id PostgreSQL.PostgreSQL.18
winget install -e --id astral-sh.uv
```

Use the newest `24.x` that `winget show --id OpenJS.NodeJS.LTS --versions` lists. You can also download the Node.js 24 installer from [nodejs.org](https://nodejs.org/en/download); if it offers **Tools for Native Modules**, tick it.

The PostgreSQL installer asks for a password for the `postgres` superuser and installs the `postgresql-x64-18` service, which starts with Windows. If winget installs it without asking, set the password with the installer from [postgresql.org](https://www.postgresql.org/download/windows/) instead.

Then turn on pnpm (as administrator), and open a new PowerShell window so the new commands are found:

```powershell
npm install -g corepack@latest
corepack enable
```

### 2. Create a database

```powershell
& "C:\Program Files\PostgreSQL\18\bin\psql.exe" -U postgres -c "CREATE USER fileship WITH PASSWORD 'change-me';"
& "C:\Program Files\PostgreSQL\18\bin\psql.exe" -U postgres -c "CREATE DATABASE fileship OWNER fileship;"
```

`psql` asks for the `postgres` password you chose in the installer.

### 3. Get the code and configure it

```powershell
cd C:\
git clone https://github.com/hanzydev/Fileship.git
cd C:\Fileship
New-Item -ItemType Directory -Force C:\FileshipData
```

Create `C:\Fileship\apps\dashboard\.env`:

```powershell
@"
DATABASE_URL=postgres://fileship:change-me@localhost:5432/fileship
NUXT_DATA_DIRECTORY=C:/FileshipData
HOST=127.0.0.1
PORT=3000
NITRO_SHUTDOWN_TIMEOUT=25000
"@ | Set-Content -Encoding ascii C:\Fileship\apps\dashboard\.env
```

Write the data directory with forward slashes and as an absolute path. Use `HOST=0.0.0.0` to reach Fileship from other devices without a proxy.

### 4. Build

```powershell
cd C:\Fileship
pnpm install --frozen-lockfile --filter dashboard... --os=current --cpu=current
pnpm --filter dashboard... --filter ml build
```

`--filter ml` sets up the [AI service](../ai.md#the-ai-service) in `appsml.venv`; Fileship starts it by itself.

### 5. Create the tables and start

```powershell
cd C:\Fileship\apps\dashboard
node node_modules\prisma\build\index.js migrate deploy
node --env-file=.env -r ./scripts/fixPrisma.js .output/server/index.mjs
```

Open `http://localhost:3000` and log in with `admin` / `password`. Stop it with Ctrl+C.

### 6. Run it as a service

Create `C:\Fileship\start.cmd`, which applies new migrations and then starts Fileship:

```bat
@echo off
cd /d C:\Fileship\apps\dashboard
node node_modules\prisma\build\index.js migrate deploy || exit /b 1
node --env-file=.env -r ./scripts/fixPrisma.js .output/server/index.mjs
```

Install it as a service with [NSSM](https://nssm.cc/) (as administrator):

```powershell
winget install -e --id NSSM.NSSM
nssm install Fileship C:\Windows\System32\cmd.exe "/c C:\Fileship\start.cmd"
nssm set Fileship AppDirectory C:\Fileship\apps\dashboard
nssm set Fileship AppStdout C:\FileshipData\fileship.log
nssm set Fileship AppStderr C:\FileshipData\fileship.log
nssm set Fileship DependOnService postgresql-x64-18
nssm start Fileship
```

The service runs as Local System and starts with Windows. `nssm restart Fileship` restarts it after an update, and `nssm remove Fileship confirm` removes it.

[PM2](https://pm2.keymetrics.io/) works too (`pm2 start` with the [ecosystem file](without-docker.md#pm2) adjusted to Windows paths), but it needs an extra package such as `pm2-windows-startup` to start with Windows.

### 7. Put a proxy in front

Use [Caddy for Windows](../proxy/caddy.md#windows) or [IIS](../proxy/iis.md), with `127.0.0.1:3000` as the target. Then follow [First steps](first-steps.md).

### Updating

```powershell
nssm stop Fileship
cd C:\Fileship
git pull
pnpm install --frozen-lockfile --filter dashboard... --os=current --cpu=current
pnpm --filter dashboard... --filter ml build
nssm start Fileship
```

`start.cmd` applies the new migrations. On the [experimental channel](../updating.md#update-channels), run `git fetch --tags` and `git checkout v6.9.0-beta.1` (the pre-release the sidebar offers) instead of `git pull`. See [Updating without Docker](../updating/without-docker.md) for what to check before an update.

## What was tested

> [!NOTE]
> The PowerShell commands for the updater secret were run in Windows PowerShell 5.1, and the winget package names were checked. The rest of this page has not been run end to end on a clean Windows 11 machine yet.
