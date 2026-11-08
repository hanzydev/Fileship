---
title: Instance settings
description: Every field of Administration → Instance, the settings super admins change for the whole Fileship instance without a restart.
---

# Instance settings

Super admins open **Administration → Instance** to change settings for everyone. Each section has its own **Save** button, and changes apply immediately, without a restart, on every open dashboard.

Every field shows its default under it (**Default: …**), which comes from the matching [environment variable](environment.md) or the built-in value. **Reset to default** next to a changed field goes back to it. Secret fields (passwords, client secret, Turnstile secret) are never shown again after saving: **Leave empty to keep** the stored value, or type a new one.

## General

| Field                 | What it does                                                                                                                                                       |
| --------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Site name**         | Shown in the browser title, link previews, emails and the dashboard. Up to 64 characters.                                                                          |
| **Dashboard address** | The public address, such as `https://files.example.com`, without a path. Links in emails and single sign-on use it, never the address a request came from.         |
| **Link protocol**     | **Same as the request**, **Always HTTPS** or **Always HTTP**: the protocol of the links Fileship returns. See [Link protocol](../proxy/overview.md#link-protocol). |

## Uploads

| Field                      | What it does                                                                                                                                                                            |
| -------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Chunk size (MB)**        | Large files are uploaded in pieces of this size, from 5 to 100 MB (25 by default). Your [reverse proxy](../proxy/overview.md#upload-size) must accept bodies a little larger than this. |
| **Avatar size limit (MB)** | Largest avatar image, 1 to 100 MB.                                                                                                                                                      |
| **Trash retention (days)** | Deleted items stay in the trash this long, 0 to 365 days. 0 deletes them immediately. See [Trash and retention](trash-and-retention.md).                                                |
| **Remove EXIF data**       | Strip location and camera details from uploaded images. Such images are re-encoded, so their bytes differ from the original.                                                            |
| **Scheduled backups**      | Let users back up their account automatically every day or every week. See [Backups](../features/backups.md).                                                                           |

## Registration

| Field                  | What it does                                                                   |
| ---------------------- | ------------------------------------------------------------------------------ |
| **Allow registration** | Show a sign up link on the login page. Off by default.                         |
| **Permissions**        | What new accounts can do: **Upload files**, **Take notes**, **Shorten links**. |
| **Storage limit (MB)** | Storage limit of new accounts; -1 means unlimited.                             |

The permissions and storage limit also apply to accounts that [single sign-on](single-sign-on.md) creates. Invites set their own. See [Registration and invites](registration.md).

## Turnstile

[Cloudflare Turnstile](https://www.cloudflare.com/products/turnstile/) protects the login and sign up forms from bots.

| Field          | What it does                                                                       |
| -------------- | ---------------------------------------------------------------------------------- |
| **Site key**   | From your Turnstile widget.                                                        |
| **Secret key** | Turnstile only turns on when both keys are set. A wrong secret blocks every login. |

## Single sign-on

**Enable single sign-on**, **Issuer URL**, **Client ID**, **Client secret**, **Provider name**, **Create accounts** and **Password and passkey login**, plus the **Redirect URI** to copy and **Test the provider**. See [Single sign-on](single-sign-on.md).

## Email

**SMTP server**, **Port**, **Encryption** (**STARTTLS**, **TLS** or **None**), **Username**, **Password** and **Sender address**, plus **Send test email**. Email features appear once the dashboard address, SMTP server and sender address are set. See [Email](email.md).

## AI

| Field                          | What it does                                                                                             |
| ------------------------------ | -------------------------------------------------------------------------------------------------------- |
| **AI features**                | Process uploads with the local AI models. Users can still turn AI off for their own files.               |
| **Preset**                     | **Low (4 GB RAM)**, **Medium (6 GB RAM)** or **High (8 GB RAM)**. See [AI presets](../ai.md#ai-presets). |
| **Parallel jobs**              | How many files are analyzed at the same time, 1 to 8.                                                    |
| **Threads per job**            | How many CPU threads each job uses, 1 to 64.                                                             |
| **Job heartbeat timeout (ms)** | A job whose server stops responding for this long is picked up again. At least 60000.                    |

Below the fields, **Queue** shows how many files are waiting, running and failed, and how much space the models use. **Retry failed** queues the failed files again, and **Reset AI data** analyzes every file again. See [AI features](../ai.md).

## Logs and telemetry

| Field                   | What it does                                                                                 |
| ----------------------- | -------------------------------------------------------------------------------------------- |
| **Log level**           | **Nothing**, **Important actions** or **Everything**. See [Logs](logs.md).                   |
| **Anonymous telemetry** | Share anonymous usage statistics to help improve Fileship. See [Telemetry](../telemetry.md). |

## Webhooks

| Field                       | What it does                                                                                                                                                                                     |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| **Allow private addresses** | Lets webhooks reach localhost, private networks and link-local addresses. Off by default. Turn it on only if you trust every user, since webhooks could then reach services inside your network. |

See [Webhooks](../webhooks.md). There is no environment variable for this field.

## Storage

The list of storages, with **Add storage**, **Set active**, **Move everything here**, **Import files** and **Import from another app**. See [Storage](storage.md) and [Moving from another app](../migrating.md).
