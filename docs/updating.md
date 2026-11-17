---
title: Updating
description: How Fileship with Docker updates itself without downtime, scheduled updates, and automatic PostgreSQL upgrades.
---

# Updating Fileship

Fileship updates itself without downtime (a database restart, when a new PostgreSQL version arrives, is the only short pause). A small built-in proxy ([Caddy](https://caddyserver.com/)) listens on port `3000` and always routes requests to a healthy container. During an update, the updater agent starts the new version in a second container (`fileship-blue` / `fileship-green`), waits until it is healthy and then stops the old one. If the new version fails to start, it is stopped and the current version keeps running. Updates also wait for any running backup restore to finish.

## One-click update

When a new release is available, admins see an update card in the sidebar. Super admins can click **Update** to start it and follow its progress live.

Fileship checks for updates when the dashboard is loaded and remembers the answer for a few minutes. A release can only be installed once its Docker images are published, usually within half an hour of its appearance on GitHub. To check again at once, open **Administration → Instance → Updates** and click **Check now**; it also says when a new version is out but not ready to install yet.

## Scheduled updates

Automatic updates are enabled by default and run every day at 3:00 AM in the updater agent's time zone (UTC unless you set `TZ` on it). You can change or disable them in `docker-compose.yml`:

```yaml
updater-agent:
    environment:
        AUTOUPDATER_ENABLED: true # set to false to disable scheduled updates
        AUTOUPDATER_CRON: '0 3 * * *' # cron expression (default: every day at 3:00 AM)
```

## Update channels

Super admins choose the channel under **Administration → Instance → Updates**:

- **Stable** (the default) installs releases. Its images are the `latest` tags.
- **Experimental** also installs pre-releases (for example `6.9.0-beta.1`) before they become stable releases. They can contain bugs. When a stable release is newer than every pre-release, the experimental channel installs it too.

Nothing is ever downgraded. If you switch from Experimental back to Stable while you run a pre-release, Fileship stays on that version until a newer stable release is out, and then updates to it.

Before you switch to Experimental:

- Back up the database and the data directory (see [Backing up the database](backups/database.md)). A pre-release can change the database, and going back to an older version needs that backup.
- Scheduled updates install pre-releases too, so turn them off if you want to choose when.
- Update from the dashboard. On the experimental channel the updater agent runs the exact pre-release images (`ghcr.io/hanzydev/fileship:6.9.0-beta.1`). Check that your `docker-compose.yml` keeps that version when you restart the stack yourself (see [Restarting with docker compose](#restarting-with-docker-compose)).

PostgreSQL follows the stable releases on both channels, so switching channels never changes the database version.

### Restarting with docker compose

On the experimental channel, and while a pre-release still runs after you switched back to Stable, the updater agent writes the version it runs into `.env` (for example `FILESHIP_VERSION=6.9.0-beta.5`). `docker-compose.yml` takes the Fileship, AI service and updater images from it, so `docker compose up -d`, `docker compose down` followed by `up -d` and `docker compose pull` keep that version. Leave the line to the agent: it changes it with every update and removes it once a stable release runs on the Stable channel, where the images are `latest` again.

A `docker-compose.yml` from before 6.9 does not read `FILESHIP_VERSION`. On the experimental channel, `docker compose up -d` then starts the stable images and Fileship goes back to an older version; **Administration → Instance → Updates** warns about this. Update the file once. On the experimental channel, first write the version Fileship runs now (the sidebar shows it), or the restart starts the stable images:

```sh
echo "FILESHIP_VERSION=6.9.0-beta.5" >> .env
git pull
docker compose up -d
```

On the stable channel, `git pull` and `docker compose up -d` are enough. Until the file is updated, restart with `docker compose restart`, which keeps the containers and their images.

## PostgreSQL

The database runs on `ghcr.io/hanzydev/fileship-postgres`, a Fileship image based on [pgautoupgrade](https://github.com/pgautoupgrade/docker-pgautoupgrade). When it starts on a data folder from an older major version, it upgrades the data in place with `pg_upgrade`, refreshes the statistics, rebuilds the indexes and then starts as usual. Fileship waits until it is healthy.

Every update first pulls the newest database image. Postgres is restarted only when its version changed (a minor release or a new major version), which takes a few seconds, or longer while a major upgrade rebuilds the indexes of a large database. If the database does not come back healthy, the update stops and the current version keeps running. After a successful update the updater agent also updates itself.

A major upgrade changes the data folder in place, so keep a copy of the `pg_data` volume (or a recent `pg_dump`) if your data matters.

### Installations from before 6.6

Older installations run `postgres:16` and an updater agent that cannot update the database or itself. Update the Compose file and both images once:

```sh
git pull
docker compose pull
docker compose up -d
```

The database is upgraded from PostgreSQL 16 to 18 during this start, and every later update is automatic. The volume is now mounted at `/var/lib/postgresql` instead of `/var/lib/postgresql/data`; if you changed the `postgres` service (for example a bind mount), change its target the same way and keep the `healthcheck` without a `test`.

## Installations from before 6.9

Since 6.9, AI runs in its own container, `fileship-ml`. One-click and scheduled updates add it by themselves: after the update the new updater agent starts the container from its own copy of the service definition, without changing your `docker-compose.yml`. If you update by hand, run `git pull` once so the Compose file has the service, then `docker compose pull` and `docker compose up -d`. Until the AI service runs, files wait for their analysis and super admins see a warning; nothing is lost and no file is analyzed again.

Running Fileship without Docker? See [Updating without Docker](updating/without-docker.md). If an update fails, see [Update problems](troubleshooting/updates.md).

Coming from 5.x? See [Upgrading to 6.0](upgrading.md).
