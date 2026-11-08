---
title: Logs
description: What Fileship records on the Logs page and in its server output, the three log levels, and who can see and clear the logs.
---

# Logs

Fileship records what people do (logins, uploads, deleted users, changed settings, …) on the **Logs** page and in its server output.

## Log level

Choose **Log level** in **Administration → Instance → Logs and telemetry** (or `NUXT_LOG_LEVEL`):

| Level                 | Value | What is recorded                                                                                                  |
| --------------------- | ----- | ----------------------------------------------------------------------------------------------------------------- |
| **Nothing**           | `0`   | Nothing.                                                                                                          |
| **Important actions** | `1`   | Account and security events, user management, settings, backups, trash, storage, AI resets, updates and webhooks. |
| **Everything**        | `2`   | All of the above plus everyday actions such as uploads, new notes, short links and file changes. The default.     |

Important actions are: logins, registrations, password resets, two-factor and passkey changes, single sign-on links, revoked sessions, impersonation, regenerated upload tokens, creating, updating and deleting users and invites, settings changes, backups (created, uploaded, restored, deleted, failed), trash actions, storages, imports, AI retries and resets, updates, cleared logs and webhooks.

## The Logs page

Admins open **Administration → Logs**. Each entry shows the user, the action, a message, the IP address and when it happened. Search and filter by action, and sort by date, action or user. New entries appear live.

Only super admins can **Clear logs**, which deletes every entry. Logs are kept until they are cleared; there is no automatic retention.

## Server output

Every recorded action is also printed to the server's output, with the IP address:

```text
2026-11-08 14:02:11 - User Admin, uploaded file a1b2c3d4.png from 203.0.113.7
```

With Docker, read it with:

```sh
docker compose logs -f fileship-blue fileship-green
```

After an update the running container may be `fileship-green`, so name both.

Without Docker, it goes to the service's log (`journalctl -u fileship` with systemd). Errors and warnings, such as failed AI jobs or storage problems, appear there too.

If every entry shows the same private address, your reverse proxy does not pass the client IP; see [Everyone has the same IP](../troubleshooting/login-and-security.md#everyone-has-the-same-ip).
