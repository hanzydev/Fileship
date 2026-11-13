---
title: Updating without Docker
description: Update a Fileship installation that runs without Docker - back up, pull the new release, rebuild, migrate and restart - and upgrade PostgreSQL yourself.
---

# Updating without Docker

Installations [without Docker](../install/without-docker.md) do not update themselves. Admins see **Update available** in the sidebar with a link to the release notes; the update itself is a few commands. Fileship is offline while it restarts, usually under a minute.

## Before you update

1. Read the release notes ([Changelog](https://github.com/hanzydev/Fileship/blob/main/CHANGELOG.md)) for anything marked as breaking.
2. Back up the database and the data directory (see [Backing up the database](../backups/database.md)).
3. Make sure no backup restore is running (**Settings → Backups** of the users who restore, or the server log).

## Update

```sh
cd /opt/fileship/app
sudo -u fileship git fetch --tags
sudo -u fileship git checkout main
sudo -u fileship git pull
sudo -u fileship pnpm install --frozen-lockfile --filter dashboard... --os=current --libc=current --cpu=current
sudo -u fileship pnpm --filter dashboard... build
sudo systemctl restart fileship
```

`main` always holds the latest stable release. To pin a release instead of following `main`, check out its tag (`sudo -u fileship git checkout v6.6.0`) instead of the `checkout main` and `pull` lines.

On the [experimental channel](../updating.md#update-channels) the sidebar also offers pre-releases. Install one by checking out its tag (`sudo -u fileship git checkout v6.9.0-beta.1`) instead of the `checkout main` and `pull` lines. Stay on pre-release tags until the sidebar offers a stable release newer than yours; going back to `main` before that would install an older version.

The systemd unit runs the database migrations before it starts. With PM2 or by hand, run them yourself before the restart:

```sh
cd /opt/fileship/app/apps/dashboard
sudo -u fileship node node_modules/prisma/build/index.js migrate deploy
```

Check that the new version is up:

```sh
curl -s http://127.0.0.1:3000/api/healthz
```

It answers `{"status":"ok","version":"…","ready":true,…}` once Fileship has started.

On Windows, see [Windows → Updating](../install/windows.md#updating).

## If the update fails

- **The build fails:** don't restart Fileship; the running process keeps serving the old version. Fix the cause (often too little memory), or check out the previous tag, install and build again, then restart.
- **A migration fails:** Fileship does not start. Read the error in `journalctl -u fileship`, then restore the database backup and go back to the previous release, or fix the cause (often a full disk or missing permissions) and restart.
- Going back to an older release after its migrations ran needs the database backup from before the update; migrations are not undone.

## Node.js and pnpm

When a release needs a newer Node.js, the release notes say so. The pnpm version comes from the repository through Corepack, so it updates with `git pull`.

## PostgreSQL

The Docker stack upgrades PostgreSQL by itself; without Docker, you do it. Minor versions (18.1 → 18.2) arrive with your system's package updates and need only a restart. For a new major version, use your distribution's tools, for example on Debian and Ubuntu:

```sh
sudo systemctl stop fileship
sudo apt install postgresql-19
sudo pg_upgradecluster 18 main
sudo systemctl start fileship
```

Check that Fileship works, then remove the old cluster with `sudo pg_dropcluster 18 main`. Fileship supports PostgreSQL 18; wait for the release notes before moving to a newer major version.
