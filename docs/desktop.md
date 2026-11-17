---
title: Fileship for Windows
description: Capture, record and upload from the Windows tray, ShareX style, and see your Fileship files in File Explorer with Fileship Harbor.
---

# Fileship for Windows

Fileship for Windows captures, records and uploads to your Fileship from the system tray, ShareX style, and shows your files in File Explorer with Fileship Harbor.

**[Download the latest version](https://github.com/hanzydev/fileship-desktop/releases/latest)** (`Fileship-Setup-<version>.exe`)

## Features

**Capture and record**

- Screenshots of the active monitor, the active window, the entire screen, a region or the last region, with hotkeys you can change: Print Screen captures the active monitor and Ctrl + Print Screen opens the region picker
- One region picker across every monitor that highlights windows and controls under the mouse, with a magnifier for pixel exact selections
- Annotations while you pick the region: shapes, arrows, pen, highlighter, text, step numbers, emoji, blur, pixelate and redact
- [Sensitive information redaction](#redacting-sensitive-information): API keys, passwords, tokens, emails, phone numbers, card numbers, IBANs and public IP addresses are blacked out before a screenshot leaves your computer
- Text capture: select a region and its text is copied to the clipboard, using your Fileship's OCR
- Screen and GIF recording with microphone (with Windows' voice processing) and system audio, pause and resume, hardware encoders and a choice of codecs and containers ([FFmpeg](https://ffmpeg.org/) is downloaded for you)

**Upload and share**

- Captures and recordings upload with your upload settings (file name style, expiry, view limit, compression, password, folder), and the link is copied to the clipboard
- Upload files from the tray, the clipboard or by dropping them on the app, and shorten links from the clipboard
- A history of everything you captured and uploaded, with links and saved files
- Optional copies on this computer, and a screenshot whose upload fails is saved there instead of being lost

**Fileship Harbor**

Fileship Harbor puts your Fileship files and folders in File Explorer. Turn it on in **Settings → Harbor**.

- Files download only when you open them, and thumbnails show without downloading
- Files you add are uploaded, renames and moves are synced, and deleted files go to the Fileship trash
- Right click and choose **Always keep on this device** to keep a copy offline
- Your notes are Markdown files in the **Notes** folder (saving one updates the note) and your short links are shortcuts in the **Links** folder
- The status button in File Explorer shows your storage use, what is syncing, how much is on this device and when it last synced, with buttons to open Fileship, upload, pause syncing, open the trash and free up space (storage use needs an up-to-date Fileship server)
- Right click a folder to copy its public link or its inbox link, and turn on **Copy links of uploaded files** to get the link of every file you drop in
- **Settings → Harbor** shows the same details, usage by type and the recent activity
- In the app's Dashboard window, **Show in Fileship Harbor** in the menu of a file, folder or note selects it in File Explorer (needs an up-to-date Fileship server)
- It needs an empty folder. Folders inside folders sync too when your Fileship server supports nested folders

**App**

- Starts with Windows and stays in the tray using only a few megabytes of memory; the app window opens on demand
- The Dashboard window downloads like a browser: it asks where to save, and the download button at the top right (or Ctrl + J) lists your downloads with their progress, opens them and shows them in their folder. With an older Fileship server there is no button, and a notification tells you when a download finishes
- English, Turkish, German, Spanish, French, Italian, Brazilian Portuguese, Russian, Azerbaijani, Japanese, Korean and Simplified Chinese
- Updates itself in the background and installs them when you restart or quit

## Redacting sensitive information

Fileship for Windows checks every screenshot for secrets and personal data with your Fileship server's [AI](ai.md) and blacks out what it finds before the screenshot is copied, saved or uploaded.

**What it blacks out**

- API keys and tokens of common services (AWS, GitHub, GitLab, Slack, Stripe, OpenAI, Anthropic, Google, npm, Discord), JSON Web Tokens, `Bearer` tokens and private key blocks
- Passwords: the value of `password = …`, `secret: …`, `api_key=…` and similar lines, and the password in connection strings such as `postgres://user:password@host`
- Email addresses and phone numbers
- Payment card numbers (checked with the Luhn algorithm), IBANs (checked with their checksum), account numbers and the machine-readable lines of passports and ID cards
- Public IP addresses. Private and local addresses (`192.168.…`, `10.…`, `127.0.0.1`, …) are left alone

Names, addresses, dates and links are not blacked out, because they are too often what you want to share.

**Region capture**

Ctrl + Print Screen opens the region picker and scans every screen first. While it scans, a border runs around the screens and the picker does not react to the mouse or the keyboard; Esc still cancels the capture. Each finding is then blacked out one after another, and you get the picker back with the redactions as editable shapes: select one to move or resize it, press Delete to remove it, or Undo to remove them all. Use the **Redact** tool (D) to black out anything the scan missed.

**Other captures**

The active monitor, the active window, the entire screen and the last region are redacted without showing anything; the notification after the upload says how many items were blacked out.

**Good to know**

- Redactions cannot be undone in the saved or uploaded image. The blocks are filled with the colors around the text, not the text itself, so tools that recover pixelated text get nothing from them
- The screenshot is sent to your own Fileship server to be scanned and is not stored there
- It needs Fileship 6.9 or newer, with AI on for the server and for your account. Without them the setting is turned off in the app
- If the scan fails (the AI service is down, the server cannot be reached), the screenshot is kept without redaction and a notification tells you
- Turn it off with **Settings → Capture → Redact sensitive information**

## Requirements

- 64-bit Windows 11, version 24H2 (build 26100) or newer
- A Fileship 6.0 server or newer

## Getting started

1. Download and run `Fileship-Setup-<version>.exe`. The installer is not code signed yet, so Windows SmartScreen may warn you: choose **More info → Run anyway**.
2. Enter your Fileship server's address and log in with your Fileship account in the window that opens.
3. Press Print Screen, or use the tray icon, to take your first capture.

The app signs in as a normal session, shown as **Fileship for Windows** in **Settings → Sessions**, where you can revoke it. Hotkeys, capture, recording, upload and Harbor settings are in the app's **Settings**.
