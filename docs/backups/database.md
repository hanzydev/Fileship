---
title: Backing up the database
description: Back up and restore a whole Fileship server - the PostgreSQL database, the data folder and secret.key - with Docker and without.
---

# Backing up the database

A Fileship server keeps its data in two places. Back up both, at the same time:

| What            | Where (Docker)                                 | Where (without Docker) |
| --------------- | ---------------------------------------------- | ---------------------- |
| The database    | the `pg_data` volume of the `postgres` service | your PostgreSQL server |
| The data folder | `data/` next to `docker-compose.yml`           | `NUXT_DATA_DIRECTORY`  |

The data folder holds the files on the local storage, thumbnails, AI models and `secret.key`. Files on S3, SFTP or FTP storage are not in it; back those up with your storage provider's tools.

> [!IMPORTANT]
> Keep `data/secret.key` with every backup. Storage credentials, upload tokens, webhook URLs and signing secrets, and the email and single sign-on secrets are encrypted with it. Without it, those values cannot be read and have to be entered again (upload tokens regenerated).

Account backups in **Settings → Backups** are something else: one user's files and items, for that user to restore. See [Backups](../features/backups.md).

## With Docker

### Back up

From the folder with `docker-compose.yml`:

```sh
docker compose exec -T postgres pg_dump -U postgres -Fc postgres > fileship-$(date +%F).dump
tar -czf fileship-data-$(date +%F).tar.gz --exclude=data/models --exclude=data/temp data
```

The dump is taken while Fileship runs and is consistent. The AI models (`data/models`) and temporary files are left out because Fileship downloads or recreates them; leave the excludes out to keep them.

To back up every night, put both lines in a script and run it from `cron`, then copy the results to another machine.

### Restore

On a fresh installation (after `docker compose up -d` has created the stack):

```sh
docker compose stop fileship-blue fileship-green
tar -xzf fileship-data-2026-11-08.tar.gz
docker compose exec -T postgres pg_restore -U postgres -d postgres --clean --if-exists < fileship-2026-11-08.dump
docker compose up -d
```

## Without Docker

### Back up

```sh
sudo -u postgres pg_dump -Fc fileship > fileship-$(date +%F).dump
sudo tar -czf fileship-data-$(date +%F).tar.gz --exclude=models --exclude=temp -C /opt/fileship data
```

### Restore

```sh
sudo systemctl stop fileship
sudo tar -xzf fileship-data-2026-11-08.tar.gz -C /opt/fileship
sudo -u postgres pg_restore -d fileship --clean --if-exists fileship-2026-11-08.dump
sudo systemctl start fileship
```

The restored database must belong to the user in `DATABASE_URL`; restore as that user (`-U fileship`) or add `--no-owner --role=fileship`.

## Moving to another server

1. Install Fileship on the new server and start it once.
2. Stop Fileship on both servers.
3. Copy the dump and the data folder over and restore them as above.
4. Point your domain at the new server.

Moving between Docker and a non-Docker installation works the same way: the database dump and the data folder are the same in both.

## Before PostgreSQL upgrades

The Docker stack upgrades PostgreSQL to new major versions by itself, in place (see [Updating](../updating.md#postgresql)). A recent dump is your way back if anything goes wrong, so keep one before updates when your data matters.
