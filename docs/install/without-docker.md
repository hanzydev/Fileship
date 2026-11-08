---
title: Install without Docker
description: Build and run Fileship directly on Linux with Node.js 24, pnpm and PostgreSQL 18, as a systemd service or with PM2.
---

# Install without Docker

Fileship can run straight on a Linux server, without Docker. You build it from the source code, run it with Node.js and give it a PostgreSQL database.

> [!IMPORTANT]
> [Docker](../installation.md) is the recommended way to run Fileship. Without Docker you give up:
>
> - **one-click and automatic updates**: you update by hand (see [Updating without Docker](../updating/without-docker.md))
> - **updates without downtime**: Fileship is offline while it restarts with the new version
> - **automatic PostgreSQL upgrades**: you upgrade PostgreSQL yourself
> - **the built-in proxy**: you set up a [reverse proxy](../proxy/overview.md) for your domain and HTTPS yourself
>
> Everything else works the same.

For Windows, see [Windows](windows.md#without-docker).

## Requirements

- A 64-bit Linux server (x86-64 or ARM64) with the [hardware for your AI preset](requirements.md). Building needs more memory than running: give the build at least 4 GB of RAM, or add swap.
- [Node.js 24](https://nodejs.org/)
- [pnpm 11](https://pnpm.io/), through Corepack
- [PostgreSQL 18](https://www.postgresql.org/), the version the Docker stack runs
- `git`, and a C++ build toolchain with Python for native modules (`build-essential` and `python3` on Debian and Ubuntu)

The commands below are for Debian 13 and Ubuntu 24.04. Other distributions work the same way with their own package names.

## 1. Install Node.js, pnpm and the build tools

```sh
sudo apt update
sudo apt install -y curl git build-essential python3 ca-certificates
curl -fsSL https://deb.nodesource.com/setup_24.x | sudo -E bash -
sudo apt install -y nodejs
sudo npm install -g corepack@latest
sudo corepack enable
```

`node --version` should print `v24`. Corepack picks the pnpm version the repository asks for.

## 2. Install PostgreSQL and create a database

Install PostgreSQL 18 from the [PostgreSQL apt repository](https://www.postgresql.org/download/linux/debian/):

```sh
sudo apt install -y postgresql-common
sudo /usr/share/postgresql-common/pgdg/apt.postgresql.org.sh
sudo apt install -y postgresql-18
```

Create a user and a database for Fileship. Choose your own password:

```sh
sudo -u postgres psql -c "CREATE USER fileship WITH PASSWORD 'change-me';"
sudo -u postgres psql -c "CREATE DATABASE fileship OWNER fileship;"
```

The connection string is then `postgres://fileship:change-me@localhost:5432/fileship`. Special characters in the password must be [percent-encoded](https://developer.mozilla.org/en-US/docs/Glossary/Percent-encoding) in it.

## 3. Create a user and get the code

Run Fileship as its own system user, in `/opt/fileship`:

```sh
sudo useradd --system --create-home --home-dir /opt/fileship --shell /usr/sbin/nologin fileship
sudo -u fileship git clone https://github.com/hanzydev/Fileship.git /opt/fileship/app
cd /opt/fileship/app
```

To install a specific release instead of the newest code, check out its tag, for example `sudo -u fileship git checkout v6.5.0`. The `main` branch is the released line.

## 4. Configure it

Create `/opt/fileship/app/apps/dashboard/.env`:

```sh
DATABASE_URL=postgres://fileship:change-me@localhost:5432/fileship
NUXT_DATA_DIRECTORY=/opt/fileship/data
HOST=127.0.0.1
PORT=3000
NITRO_SHUTDOWN_TIMEOUT=25000
```

Then make it readable only by Fileship:

```sh
sudo chown fileship:fileship /opt/fileship/app/apps/dashboard/.env
sudo chmod 600 /opt/fileship/app/apps/dashboard/.env
```

- `NUXT_DATA_DIRECTORY` holds uploads, thumbnails, AI models, temporary files and `secret.key`. Always give an absolute path: a relative one is resolved two folders above the folder Fileship starts in, which is easy to get wrong.
- `HOST=127.0.0.1` keeps Fileship reachable only from the machine itself, for a reverse proxy on the same server. Use `0.0.0.0` to reach it from other machines.
- Every other setting can be changed later in **Administration → Instance**. [Environment variables](../configuration/environment.md) lists them all.

## 5. Build

```sh
cd /opt/fileship/app
sudo -u fileship pnpm install --frozen-lockfile --filter dashboard... --os=current --libc=current --cpu=current
sudo -u fileship pnpm --filter dashboard... build
```

The first command installs only what the dashboard needs, for this system's processor. The build takes a few minutes and writes the server to `apps/dashboard/.output`.

## 6. Create the database tables

```sh
cd /opt/fileship/app/apps/dashboard
sudo -u fileship node node_modules/prisma/build/index.js migrate deploy
```

This reads `DATABASE_URL` from the `.env` file. Run it again after every update; it only applies what is new.

## 7. Start it

Try it in the foreground first:

```sh
cd /opt/fileship/app/apps/dashboard
sudo -u fileship node --env-file=.env -r ./scripts/fixPrisma.js .output/server/index.mjs
```

Open `http://<server>:3000` (or `http://localhost:3000` on the server itself) and log in with `admin` / `password`. Stop it with Ctrl+C, then set it up as a service.

### systemd

Create `/etc/systemd/system/fileship.service`:

```ini
[Unit]
Description=Fileship
After=network-online.target postgresql.service
Wants=network-online.target

[Service]
Type=simple
User=fileship
Group=fileship
WorkingDirectory=/opt/fileship/app/apps/dashboard
EnvironmentFile=/opt/fileship/app/apps/dashboard/.env
ExecStartPre=/usr/bin/node node_modules/prisma/build/index.js migrate deploy
ExecStart=/usr/bin/node -r ./scripts/fixPrisma.js .output/server/index.mjs
Restart=on-failure
RestartSec=5
TimeoutStopSec=30
Environment=CHECKPOINT_DISABLE=1

[Install]
WantedBy=multi-user.target
```

Start it and have it start at boot:

```sh
sudo systemctl daemon-reload
sudo systemctl enable --now fileship
sudo journalctl -u fileship -f
```

`ExecStartPre` applies new database migrations each time Fileship starts, like the Docker image does. `CHECKPOINT_DISABLE=1` stops Prisma from checking for its own updates at every start.

### PM2

If you already use [PM2](https://pm2.keymetrics.io/), create `/opt/fileship/app/apps/dashboard/ecosystem.config.cjs`:

```js
module.exports = {
    apps: [
        {
            name: 'fileship',
            cwd: '/opt/fileship/app/apps/dashboard',
            script: '.output/server/index.mjs',
            node_args: '--env-file=.env -r ./scripts/fixPrisma.js',
            kill_timeout: 30000,
        },
    ],
};
```

Run the migration from [step 6](#6-create-the-database-tables) yourself before each start, then:

```sh
pm2 start ecosystem.config.cjs
pm2 save
pm2 startup
```

Run Fileship as **one** process. Do not use PM2's cluster mode or start several copies: live updates (socket.io) only reach people connected to the same process.

## 8. Next steps

1. Put a [reverse proxy](../proxy/overview.md) in front of it for your domain and HTTPS. The proxy sends requests to `127.0.0.1:3000`.
2. Follow [First steps](first-steps.md): change the admin password and set the dashboard address.
3. Back up the database and the data directory (see [Backing up the database](../backups/database.md)). Keep `secret.key` from the data directory with them: storage credentials, upload tokens, webhook URLs and email and single sign-on secrets are encrypted with it.

> [!NOTE]
> A test of this page from start to finish on clean Ubuntu 24.04 and Debian 13 systems is still to be done.

## Notes

- **ffmpeg** for video thumbnails and **PDFium** for PDF previews come with the build; nothing else needs to be installed.
- **AI models** are downloaded from Hugging Face into `<data directory>/models` the first time they are needed. See [AI features](../ai.md).
- **Updates:** admins still see **Update available** in the sidebar, with a link to the release, but there is no **Update** button. See [Updating without Docker](../updating/without-docker.md).
- **Client IP:** Fileship trusts `X-Forwarded-For` only from private and loopback addresses, which covers a proxy on the same machine. See [Client IP](../proxy/overview.md#client-ip).
