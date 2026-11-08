---
title: Update problems
description: Fix Fileship updates - "Couldn't update Fileship", an unreachable updater agent, a wrong SECRET_TOKEN, an update that waits for PostgreSQL or a backup restore.
---

# Update problems

Updates with Docker are done by the `updater-agent` service. Its output says what happened:

```sh
docker compose logs --tail=200 updater-agent
```

## No Update button

- Only super admins see **Update**; admins only see **Update available**.
- Without the updater agent (an installation [without Docker](../install/without-docker.md), or a stack without the `updater-agent` service), there is no button. Update by hand.
- The dashboard cannot reach the agent: see the next section.

## The dashboard cannot reach the updater agent

The sidebar shows **Update available** with a link but no **Update** button, and the agent's log shows `401` or nothing:

- `.env` next to `docker-compose.yml` must contain `SECRET_TOKEN=…`, and both services read it (`NUXT_UPDATER_SECRET: ${SECRET_TOKEN}` and `SECRET_TOKEN: ${SECRET_TOKEN}`). After creating or changing it, run `docker compose up -d`.
- On Windows, a `.env` written as UTF-16 is not read; see [Windows](../install/windows.md#3-generate-the-updater-secret).
- `NUXT_UPDATER_URL` must stay `http://updater-agent:3001`.

## Couldn't update Fileship

**Update failed, the current version keeps running.** The new version was started next to the old one and did not become healthy within three minutes, or exited. The old version keeps serving. Look at the new container's output:

```sh
docker compose logs --tail=200 fileship-green fileship-blue
```

Common causes: a full disk, too little memory to run two versions side by side for a moment, or a database migration that failed.

## The update waits for PostgreSQL

When a new PostgreSQL version arrives, the agent restarts the database and waits until it is healthy, up to an hour. A major upgrade of a large database (`pg_upgrade` and rebuilding indexes) takes a while; `docker compose logs postgres` shows its progress. If the database does not become healthy, the update stops and Fileship keeps running on the current version.

## The update waits for a backup restore

Updates wait (up to 10 minutes) while a user restores a backup, so the restore is not cut off. Scheduled updates skip their run and try again the next day.

## Scheduled updates don't run

- `AUTOUPDATER_ENABLED` must be `true` on the `updater-agent` service.
- `AUTOUPDATER_CRON` runs in the agent's time zone, UTC unless you set `TZ`.
- A new release is only installed once its image is published, usually within an hour of the release.

## Installations from before 6.6

Older stacks run `postgres:16` and an agent that cannot update the database or itself. Run `git pull`, `docker compose pull` and `docker compose up -d` once; see [Updating](../updating.md#installations-from-before-66).

## git pull refuses to update

`error: Your local changes to the following files would be overwritten by merge: docker-compose.yml` means you changed the file. Commit your changes (`git commit -am "Local settings"`) and run `git pull` again, which merges them.
