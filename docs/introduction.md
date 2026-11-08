---
title: Introduction
description: What Fileship is, the parts it is made of, and where to start reading.
---

# Introduction

Fileship is an open-source, self-hosted platform for your files, screenshots, notes and short links. You run it on your own server, upload from the browser, ShareX, the Windows app, the browser extension or your phone, and share links that you control.

## What it does

- **Files and folders:** upload anything, organize it in nested folders, preview images, videos, PDFs, code and Markdown, and share folders with other people as viewers or editors. See [Files](features/files.md) and [Folders](features/folders.md).
- **Sharing:** every file has a link. Protect it with a password, an expiry or a view limit, and choose how it looks on Discord and other apps ([Sharing options](features/sharing.md), [Link previews](link-previews.md)).
- **Notes and short links:** Markdown notes and short links with click counts ([Notes](features/notes.md), [Short links](features/short-links.md)).
- **Inbox:** a page where anyone can send you files, without an account ([Inbox](features/inbox.md)).
- **AI on your server:** smart search in many languages, captions, OCR and sensitive content detection, with models that run on your own CPU ([AI features](ai.md)).
- **Accounts:** users with permissions and storage limits, passkeys, two-factor authentication, single sign-on, invites and email ([Users and permissions](features/users.md)).
- **Safety nets:** a trash, account backups on a schedule and duplicate detection ([Trash](features/trash.md), [Backups](features/backups.md), [Duplicates](features/duplicates.md)).
- **Storage:** your server's disk, S3-compatible storage, SFTP or FTP ([Storage](configuration/storage.md)).
- **Integrations:** webhooks to Discord, Slack or your own service ([Webhooks](webhooks.md)).
- **12 languages:** English, Turkish, German, Spanish, French, Italian, Brazilian Portuguese, Russian, Azerbaijani, Japanese, Korean and Simplified Chinese.

## The pieces

| Piece                                     | What it is                                                                                   |
| ----------------------------------------- | -------------------------------------------------------------------------------------------- |
| Dashboard                                 | The web app and its API. Everything else talks to it.                                        |
| [Fileship for Windows](desktop.md)        | Captures, records and uploads from the tray, and shows your files in File Explorer (Harbor). |
| [Browser extension](browser-extension.md) | Uploads images and screenshots and shortens links from Chrome, Edge and Firefox.             |
| [Phone](mobile.md)                        | The dashboard installed as an app, with the Android share sheet.                             |
| [Uploaders](uploaders/sharex.md)          | ShareX, scripts for macOS and Linux, `curl` and anything that can send an HTTP request.      |

## Where to start

1. Check the [requirements](install/requirements.md).
2. Install with [Docker](installation.md) (recommended), [without Docker](install/without-docker.md) or on [Windows](install/windows.md).
3. Follow [First steps](install/first-steps.md).
4. Put it on your domain with a [reverse proxy](proxy/overview.md).

Coming from Zipline, XBackBone or Chibisafe? See [Moving from another app](migrating.md). Something not working? See [Troubleshooting](troubleshooting/index.md).
