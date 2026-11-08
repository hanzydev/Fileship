---
title: First steps
description: What to do after installing Fileship - secure the admin account, set the dashboard address, add users and connect your uploaders.
---

# First steps

You installed Fileship with [Docker](../installation.md), [without Docker](without-docker.md) or on [Windows](windows.md), and it opens at `http://<server>:3000`. These steps take a few minutes.

## 1. Secure the admin account

Log in with `admin` / `password`, then open **Settings → Security**:

1. Under **Password**, set a new password with **Change password**.
2. Optionally turn on the **Authenticator app** or add a **Passkey**. Passkeys only work over HTTPS (or on `http://localhost`).

The seed account is a **Super admin**: it can change instance settings, manage storage and other admins. To use another username, change it in **Settings → Profile**.

## 2. Put it on your domain

Set up a [reverse proxy](../proxy/overview.md) with HTTPS, then open **Administration → Instance → General** and set:

- **Dashboard address**: the public address, for example `https://files.example.com`. Emails and single sign-on build their links from it.
- **Link protocol**: leave it on **Same as the request** unless your links come out with the wrong protocol.

**Site name** is shown in the browser title, link previews and emails.

## 3. Review the instance settings

Still in **Administration → Instance**, go through the sections once. The defaults are fine for most servers; [Instance settings](../configuration/instance-settings.md) explains each field.

- **AI**: on by default with the `medium` preset (6 GB RAM). On a small server, choose **Low (4 GB RAM)** or turn **AI features** off. See [AI features](../ai.md).
- **Uploads**: chunk size, EXIF removal and how long deleted items stay in the trash.
- **Email**: needed for password reset links, invites and storage warnings. See [Email](../configuration/email.md).
- **Storage**: keep files on the server's disk (the default) or add S3, SFTP or FTP. See [Storage](../configuration/storage.md).

## 4. Add people

Open **Administration → Users**:

- **New user** creates an account with a username, a password, permissions and limits.
- **Invite** sends an email with a link to sign up (needs [email](../configuration/email.md)).

Or let people sign up themselves, or log in through your identity provider. See [Registration and invites](../configuration/registration.md), [Single sign-on](../configuration/single-sign-on.md) and [Users and permissions](../features/users.md).

## 5. Connect your uploaders

Open **Settings → Integrations**. The **Upload token** there lets tools upload files and shorten links for you; set your **Upload options** and download a config:

- [ShareX](../uploaders/sharex.md) on Windows, or [Fileship for Windows](../desktop.md)
- the script for [macOS](../uploaders/macos.md) or [Linux](../uploaders/linux.md)
- the [browser extension](../browser-extension.md)
- [your phone](../mobile.md)
- anything else through the [upload API](../api/uploads.md)

## 6. Plan backups

Fileship's own account backups (**Settings → Backups**) are per user. To protect the whole server, back up the database and the data folder, including `secret.key`. See [Backing up the database](../backups/database.md).
