---
title: Linux
description: Capture a region on Linux (Wayland or X11) and upload it to Fileship with the generated shell script, using grim, Flameshot, maim, GNOME Screenshot or Spectacle.
---

# Linux

Fileship generates a shell script for Linux that captures a region of the screen (or uploads the files you give it), copies the link and shows a notification.

## Install the tools

The script needs `curl` and `file`, one screenshot tool and, for the clipboard, `wl-clipboard` (Wayland) or `xclip` / `xsel` (X11). `notify-send` shows the notification when it is installed.

| Screenshot tool    | Desktop                       | Debian / Ubuntu package |
| ------------------ | ----------------------------- | ----------------------- |
| `grim` and `slurp` | Sway, Hyprland, other wlroots | `grim slurp`            |
| `flameshot`        | any                           | `flameshot`             |
| `maim`             | X11                           | `maim`                  |
| `gnome-screenshot` | GNOME                         | `gnome-screenshot`      |
| `spectacle`        | KDE Plasma                    | `kde-spectacle`         |

For example on Ubuntu with GNOME on Wayland:

```sh
sudo apt install curl file gnome-screenshot wl-clipboard libnotify-bin
```

The script uses the first tool it finds, in the order of the table.

## Get the script

1. Open **Settings → Integrations**, set your **Upload options** and choose **Download** on the **Linux** card. You get `fileship.sh` with your address, upload token and options inside.
2. Make it executable and put it on your `PATH`:

    ```sh
    chmod +x ~/Downloads/fileship.sh
    mkdir -p ~/.local/bin
    mv ~/Downloads/fileship.sh ~/.local/bin/fileship
    ```

## Use it

```sh
fileship                     # capture a region and upload it
fileship report.pdf a.png    # upload files
fileship --link https://example.com/long   # shorten a link
```

With files, each is uploaded and the links are copied together, separated by spaces. `--link` is there when your account can shorten links.

## Keyboard shortcut

Bind the command to a key in your desktop's settings:

- **GNOME:** **Settings → Keyboard → View and Customize Shortcuts → Custom Shortcuts**, command `/home/<you>/.local/bin/fileship`.
- **KDE Plasma:** **System Settings → Shortcuts → Add New → Command or Script**.
- **Sway / Hyprland:** `bindsym Print exec ~/.local/bin/fileship` (Sway) or `bind = , Print, exec, ~/.local/bin/fileship` (Hyprland).

For Flameshot's own upload button and its GUI, see [Flameshot](flameshot.md).

## Things to know

- The script sends each file in one request; your [reverse proxy](../proxy/overview.md#upload-size) must accept bodies as large as the file.
- The token is stored in the script in plain text. Keep it private, and download the script again after **Regenerate**.

> [!NOTE]
> Not tested yet on a Linux desktop: the capture tools, the clipboard and the shortcuts.
