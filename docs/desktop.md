# Fileship for Windows

Fileship for Windows captures, records and uploads to your Fileship from the system tray, ShareX style, and shows your files in File Explorer with Fileship Harbor.

**[Download the latest version](https://github.com/hanzydev/fileship-desktop/releases/latest)** (`Fileship-Setup-<version>.exe`)

## Features

**Capture and record**

- Screenshots of the active monitor, the active window, the entire screen, a region or the last region, with hotkeys you can change: Print Screen captures the active monitor and Ctrl + Print Screen opens the region picker
- One region picker across every monitor that highlights windows and controls under the mouse, with a magnifier for pixel exact selections
- Annotations while you pick the region: shapes, arrows, pen, highlighter, text, step numbers, emoji, blur and pixelate
- Text capture: select a region and its text is copied to the clipboard, using your Fileship's OCR
- Screen and GIF recording with microphone and system audio, pause and resume, hardware encoders and a choice of codecs and containers ([FFmpeg](https://ffmpeg.org/) is downloaded for you)

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
- It needs an empty folder, and folders inside folders are not synced, because Fileship folders are flat

**App**

- Starts with Windows and stays in the tray
- English, Turkish, German, French, Italian, Azerbaijani, Japanese, Korean and Simplified Chinese
- Updates itself in the background and installs them when you restart or quit

## Requirements

- Windows 10 or 11, 64-bit
- A Fileship 6.0 server or newer

## Getting started

1. Download and run `Fileship-Setup-<version>.exe`. The installer is not code signed yet, so Windows SmartScreen may warn you: choose **More info → Run anyway**.
2. Enter your Fileship server's address and log in with your Fileship account in the window that opens.
3. Press Print Screen, or use the tray icon, to take your first capture.

The app signs in as a normal session, shown as **Fileship for Windows** in **Settings → Sessions**, where you can revoke it. Hotkeys, capture, recording, upload and Harbor settings are in the app's **Settings**.
