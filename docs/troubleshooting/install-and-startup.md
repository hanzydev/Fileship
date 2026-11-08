---
title: Installation and startup problems
description: Fix a Fileship container that keeps restarting, a health check that stays starting, a data folder that is not writable or a lost secret.key.
---

# Installation and startup problems

Start by reading the server output. With Docker:

```sh
docker compose ps
docker compose logs --tail=200 fileship-blue fileship-green postgres
```

Without Docker: `journalctl -u fileship -n 200` (systemd) or `pm2 logs fileship`.

## The Fileship container keeps restarting

`docker compose ps` shows `fileship-blue` as `Restarting` or `unhealthy`. The log says why:

| In the log                                         | Cause and fix                                                                                                                                 |
| -------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
| `P1001: Can't reach database server`               | Postgres is not up yet or `DATABASE_URL` points to the wrong host. With Compose the host is `postgres`. Check `docker compose logs postgres`. |
| `P1000: Authentication failed`                     | The user or password in `DATABASE_URL` is wrong. With Compose, keep the defaults unless you changed `POSTGRES_PASSWORD` too.                  |
| `Error: P3009` / `migrate found failed migrations` | A migration failed earlier, often because the disk was full. Free space, restore the database backup, then start again.                       |
| `EACCES: permission denied` on `/fileship/data`    | See [The data folder is not writable](#the-data-folder-is-not-writable).                                                                      |
| `ENOSPC: no space left on device`                  | The disk is full. Free space; AI models alone take up to 3 GB.                                                                                |
| killed, exit code 137                              | Out of memory. Choose a smaller [AI preset](../ai.md#ai-presets) or turn AI off, or add RAM. See [AI problems](ai.md#out-of-memory).          |

## The page does not load

- **`docker compose ps` shows everything running, but `http://<server>:3000` times out:** a firewall blocks port 3000, or you are using the wrong address. Try `curl http://localhost:3000/api/healthz` on the server itself.
- **Bad Gateway or "no upstreams available" from the built-in proxy:** Fileship is still starting. The first start creates the database and can take a minute or two; wait for the health check.
- **It works on the server, not through your domain:** see [Reverse proxy overview](../proxy/overview.md).

## The health check stays "starting"

`/api/healthz` answers `{"status":"starting","ready":false}` with status 503 until Fileship has finished starting: migrations, folders, storage checks and the search index. On a large instance, building the search index can take a while. If it never becomes ready, the log shows the error that stopped it.

`docker compose ps` shows `health: starting` for the first 30 seconds anyway.

## The data folder is not writable

Fileship needs to write to its data folder (`data/` next to `docker-compose.yml`, or `NUXT_DATA_DIRECTORY`). With Docker the container runs as root, so this only fails when the folder sits on a mount that refuses writes (some NFS and SMB shares, read-only disks). Without Docker, give the folder to the user Fileship runs as:

```sh
sudo chown -R fileship:fileship /opt/fileship/data
```

## Files are missing after a restart

On every start, Fileship removes files whose bytes are missing from their storage. A storage that cannot be reached or looks empty is skipped, so nothing is deleted because of a temporary problem. If you moved the data folder, make sure `NUXT_DATA_DIRECTORY` (or the Compose volume `./data:/fileship/data`) points at the new place before starting.

Without Docker, a relative `NUXT_DATA_DIRECTORY` is resolved two folders above the folder Fileship starts in. Starting it from another folder makes it use another data folder that looks empty. Always use an absolute path.

## Lost secret.key

`data/secret.key` encrypts storage credentials, upload tokens, webhook URLs and secrets, and the email and single sign-on secrets. If it is lost, Fileship creates a new one, and the old values cannot be read:

- edit each storage and enter its secret again
- each user regenerates their upload token in **Settings → Integrations**
- edit each webhook and enter its URL (and secret) again
- enter the SMTP password and the single sign-on client secret again in **Administration → Instance**

Files, accounts and everything else are not affected. Keep `secret.key` in your [backups](../backups/database.md).

## Default login doesn't work

`admin` / `password` is only created when the database has no users. If you restored a database or the password was changed, use **Forgot your password?** (needs [email](../configuration/email.md) and a confirmed address), or have another super admin set a new password on the **Users** page.

## Windows-specific problems

- **`docker compose` says the `.env` file is invalid:** it was written as UTF-16. Write it again with `Set-Content -Encoding ascii` (see [Windows](../install/windows.md#3-generate-the-updater-secret)).
- **Docker Desktop is not running after a reboot:** turn on **Start Docker Desktop when you sign in to your computer**; Docker Desktop only runs while someone is signed in.
