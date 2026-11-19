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
- Annotations while you pick the region: shapes, arrows, pen, highlighter, text, step numbers, emoji, blur and pixelate
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

## Requirements

- 64-bit Windows 11, version 24H2 (build 26100) or newer
- A Fileship 6.0 server or newer

## Getting started

1. Download and run `Fileship-Setup-<version>.exe`. The installer is not code signed yet, so Windows SmartScreen may warn you: choose **More info → Run anyway**.
2. Enter your Fileship server's address and log in with your Fileship account in the window that opens.
3. Press Print Screen, or use the tray icon, to take your first capture.

The app signs in as a normal session, shown as **Fileship for Windows** in **Settings → Sessions**, where you can revoke it. Hotkeys, capture, recording, upload and Harbor settings are in the app's **Settings**.

## Fileship for Linux

The same app runs on Linux: capture, annotations, text capture, recording, uploads, the history and Fileship Harbor. It needs a 64-bit (x86-64) system and a Fileship 6.0 server or newer.

**[Download the latest version](https://github.com/hanzydev/fileship-desktop/releases/latest)**

| Package                  | For                                           | Install                                     |
| ------------------------ | --------------------------------------------- | ------------------------------------------- |
| `Fileship-<version>.deb` | Ubuntu 24.04, Linux Mint 22, Debian 13, newer | `sudo apt install ./Fileship-<version>.deb` |
| `Fileship-<version>.rpm` | Fedora 43 and newer                           | `sudo dnf install ./Fileship-<version>.rpm` |

It is made for GNOME 48 and KDE Plasma 6 on Wayland, and for X11 sessions such as Cinnamon and XFCE. Start **Fileship** from your applications and log in; it then starts with your session and stays in the tray.

Linux desktops work differently from Windows in a few places:

- **Tray.** GNOME shows tray icons only with the AppIndicator extension, which Ubuntu ships. Without it, open Fileship from your applications; everything in the tray menu is also in the app.
- **Hotkeys.** On X11 they work like on Windows. On Wayland an app cannot take keys itself: GNOME 48 and KDE Plasma ask you to confirm the hotkeys in a system dialog, and you can change them in the system settings. On other desktops, **Settings → Hotkeys** shows a command for each action that you bind in your desktop's keyboard settings.
- **Screenshots.** On Wayland your system asks once which screens Fileship may capture, and only those can be captured; **Settings → Capture → Choose screens again** asks again. The region picker cannot see windows there, so it does not snap to them, and capturing the active window opens your system's window picker. On X11 the picker snaps to windows.
- **Recording.** Fileship records with the FFmpeg of your system, so install the `ffmpeg` package. On X11 the recording toolbar and border sit next to the recorded area; on Wayland, and when there is no room next to the area, a notification has the Stop, Pause and Discard buttons. Noise suppression, echo cancellation and gain control are not available.
- **Fileship Harbor.** Your files appear in the folder you choose, in any file manager, and download when you open them. The folder is empty while Fileship is not running, opened files stay downloaded until you choose **Free up space**, hidden files (names that start with a dot) are not synced, and everything you delete goes to the Fileship trash. Nautilus (with the `nautilus-python` package), Nemo and Dolphin get **Upload to Fileship** and the Harbor entries in their right-click menus. There are no sync status icons and no **Always keep on this device**.
- **Updates.** They download in the background. **Restart now** installs the new package with your system's package manager, so your system asks for your password.
- **Login.** Your session is kept in your keyring (GNOME Keyring or KWallet), so logging in needs one.

The session appears as **Fileship for Linux** in **Settings → Sessions** on an up-to-date Fileship server.
