---
title: Updating
description: How Fileship with Docker updates itself without downtime, scheduled updates, and automatic PostgreSQL upgrades.
---

# Updating Fileship

Fileship updates itself without downtime (a database restart, when a new PostgreSQL version arrives, is the only short pause). A small built-in proxy ([Caddy](https://caddyserver.com/)) listens on port `3000` and always routes requests to a healthy container. During an update, the updater agent starts the new version in a second container (`fileship-blue` / `fileship-green`), waits until it is healthy and then stops the old one. If the new version fails to start, it is stopped and the current version keeps running. Updates also wait for any running backup restore to finish.

## One-click update

When a new release is available, admins see an update card in the sidebar. Super admins can click **Update** to start it and follow its progress live.

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

- **Stable** (the default) installs releases. Its images are the `latest` tags in `docker-compose.yml`.
- **Experimental** also installs pre-releases (for example `6.9.0-beta.1`) before they become stable releases. They can contain bugs. When a stable release is newer than every pre-release, the experimental channel installs it too.

Nothing is ever downgraded. If you switch from Experimental back to Stable while you run a pre-release, Fileship stays on that version until a newer stable release is out, and then updates to it.

Before you switch to Experimental:

- Back up the database and the data directory (see [Backing up the database](backups/database.md)). A pre-release can change the database, and going back to an older version needs that backup.
- Scheduled updates install pre-releases too, so turn them off if you want to choose when.
- Update from the dashboard only. On the experimental channel the updater agent runs the exact pre-release images (`ghcr.io/hanzydev/fileship:6.9.0-beta.1`), while a `docker compose pull` or `docker compose up -d` you run yourself uses the `latest` images of `docker-compose.yml`, which are the stable release.

PostgreSQL follows the stable releases on both channels, so switching channels never changes the database version.

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

Running Fileship without Docker? See [Updating without Docker](updating/without-docker.md). If an update fails, see [Update problems](troubleshooting/updates.md).

Coming from 5.x? See [Upgrading to 6.0](upgrading.md).
