# Fileship for Windows and Linux

Fileship for Windows and Fileship for Linux capture, record and upload to your Fileship from the system tray, ShareX style, and show your files in your file manager with Fileship Harbor.

**[Download the latest version](https://github.com/hanzydev/fileship-desktop/releases/latest)**: `Fileship-Setup-<version>.exe` for Windows, or for Linux an AppImage, a `.deb`, an `.rpm` or a `.tar.gz` ([which one](#installing-on-linux)).

## Features

**Capture and record**

- Screenshots of the active monitor, the active window, the entire screen, a region or the last region, with hotkeys you can change: Print Screen captures the active monitor and Ctrl + Print Screen opens the region picker
- One region picker across every monitor, with a magnifier for pixel exact selections. It highlights windows and controls under the mouse on Windows, and windows on Linux with X11
- Annotations while you pick the region: shapes, arrows, pen, highlighter, text, step numbers, emoji, blur and pixelate
- Text capture: select a region and its text is copied to the clipboard, using your Fileship's OCR
- Screen and GIF recording with microphone and system audio, pause and resume, hardware encoders and a choice of codecs and containers ([FFmpeg](https://ffmpeg.org/) is downloaded for you). On Windows the microphone can use Windows' voice processing

**Upload and share**

- Captures and recordings upload with your upload settings (file name style, expiry, view limit, compression, password, folder), and the link is copied to the clipboard
- Upload files from the tray, the clipboard, your file manager's menu or by dropping them on the app, and shorten links from the clipboard
- A history of everything you captured and uploaded, with links and saved files
- Optional copies on this computer, and a screenshot whose upload fails is saved there instead of being lost

**Fileship Harbor**

Fileship Harbor puts your Fileship files and folders in File Explorer on Windows, and in a folder you open in your file manager on Linux. Turn it on in **Settings → Harbor**.

- Files download only when you open them
- Files you add are uploaded, renames and moves are synced, and deleted files go to the Fileship trash
- Your notes are Markdown files in the **Notes** folder (saving one updates the note) and your short links are shortcuts in the **Links** folder
- Right click a folder to copy its public link or its inbox link, and turn on **Copy links of uploaded files** to get the link of every file you drop in
- **Settings → Harbor** shows your storage use, what is syncing, usage by type and the recent activity, and the tray menu pauses syncing and frees up space
- It needs an empty folder. Folders inside folders sync too when your Fileship server supports nested folders

On Windows, thumbnails show without downloading, **Always keep on this device** in the right click menu keeps a copy offline, and the status button in File Explorer shows your storage use with buttons to open Fileship, upload, pause syncing, open the trash and free up space (storage use needs an up-to-date Fileship server).

**App**

- Starts when you log in and stays in the tray using only a few megabytes of memory; the app window opens on demand
- English, Turkish, German, Spanish, French, Italian, Brazilian Portuguese, Russian, Azerbaijani, Japanese, Korean and Simplified Chinese
- Updates itself in the background and installs them when you restart or quit (on Linux the AppImage, `.deb` and `.rpm` do; the AUR package updates through your package manager)

## Requirements

- **Windows:** 64-bit Windows 11, version 24H2 (build 26100) or newer
- **Linux:** a 64-bit (x86-64) distribution with glibc 2.39 and GTK 4.14 or newer, such as Ubuntu 24.04, Debian 13, Fedora 41, openSUSE Leap 16 or Tumbleweed and Arch Linux, with an X11 or Wayland session
- A Fileship 6.0 server or newer

## Getting started

1. Install the app: on Windows, download and run `Fileship-Setup-<version>.exe`. The installer is not code signed yet, so Windows SmartScreen may warn you: choose **More info → Run anyway**. On Linux, see [Installing on Linux](#installing-on-linux).
2. Enter your Fileship server's address and log in with your Fileship account in the window that opens.
3. Press Print Screen, or use the tray icon, to take your first capture.

The app signs in as a normal session, shown as **Fileship for Windows** or **Fileship for Linux** in **Settings → Sessions**, where you can revoke it. Hotkeys, capture, recording, upload and Harbor settings are in the app's **Settings**.

## Installing on Linux

| Package                                                           | For                                 | Install                                                                                                                          |
| ----------------------------------------------------------------- | ----------------------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| `fileship_<version>_amd64.deb`                                    | Ubuntu, Debian, Linux Mint, Pop!_OS | `sudo apt install ./fileship_<version>_amd64.deb`                                                                                |
| `fileship-<version>.x86_64.rpm`                                   | Fedora, openSUSE                    | `sudo dnf install ./fileship-<version>.x86_64.rpm` or `sudo zypper install --allow-unsigned-rpm ./fileship-<version>.x86_64.rpm` |
| `Fileship-<version>-x86_64.AppImage`                              | any distribution                    | `chmod +x Fileship-<version>-x86_64.AppImage` and run it                                                                         |
| [`fileship-bin`](https://aur.archlinux.org/packages/fileship-bin) | Arch Linux                          | `yay -S fileship-bin` or another AUR helper                                                                                      |
| `fileship-<version>-x64.tar.gz`                                   | anything else                       | extract it and run `fileship`                                                                                                    |

The `.deb` and `.rpm` pull in everything the app needs. With the AppImage or the `.tar.gz`, install GTK 4, libcurl, libsecret, PulseAudio's client library (also part of PipeWire), libpipewire and fuse3 with your package manager if they are missing.

The app keeps its settings in `~/.config/Fileship`. The AppImage copies its background part to `~/.local/share/Fileship/runtime`, so it keeps running after the AppImage's window closes.

On X11, a shortcut that another app already uses is marked in **Settings → Hotkeys**. GNOME and KDE Plasma take Print Screen for their own screenshot tools, so free it in the desktop's keyboard settings or pick other keys.

### Wayland

Wayland desktops keep apps from reading the screen and the keyboard, so on Wayland Fileship goes through the desktop's portals:

- **Screenshots** use the Screenshot portal. GNOME asks for permission the first time, while a Fileship window is open.
- **The region picker** cannot highlight windows; a click picks the monitor under the mouse.
- **Hotkeys** use the Global Shortcuts portal (KDE Plasma, GNOME 48 or newer, Hyprland), which asks you to confirm them. Where it is missing, **Settings → Hotkeys** lists commands such as `fileship --action region` that you can bind to keys in your desktop's keyboard settings.
- **Recordings** use the ScreenCast portal and PipeWire, so your desktop asks which screen to share. A recording stays within one monitor, and the toolbar and the border around the area are not shown.

### Tray icon

Fileship lives in the system tray through the StatusNotifierItem standard, which KDE Plasma, Cinnamon, Xfce and most desktops show. GNOME needs the [AppIndicator and KStatusNotifierItem Support](https://extensions.gnome.org/extension/615/appindicator-support/) extension, which Ubuntu and Fedora's GNOME include or offer. Without a tray, use the hotkeys, the app menu entry and `fileship --action <name>`.

### Fileship Harbor on Linux

Harbor mounts your files in the folder you choose with FUSE, so it needs `fuse3`. Files you open are kept in `~/.cache/Fileship/harbor` until you free up space. To keep a file offline, run `setfattr -n user.fileship.pinned -v 1 <file>`.

Nautilus, Nemo and Dolphin get **Upload to Fileship** and the Harbor actions (copy link, open in the browser, show in Fileship, copy the folder or inbox link) in their right click menus. In Nautilus they are under **Scripts → Fileship**.
