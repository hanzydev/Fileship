---
title: macOS
description: Capture a region on macOS and upload it to Fileship with the generated shell script, bound to a keyboard shortcut through Shortcuts.
---

# macOS

Fileship generates a shell script for macOS that captures a region of the screen (or uploads the files you give it), copies the link and shows a notification. It uses only tools macOS ships with: `screencapture`, `curl`, `pbcopy` and `osascript`.

## Get the script

1. Open **Settings → Integrations**, set your **Upload options** and choose **Download** on the **macOS** card. You get `fileship.sh` with your address, upload token and options inside.
2. Make it executable:

    ```sh
    chmod +x ~/Downloads/fileship.sh
    mkdir -p ~/bin
    mv ~/Downloads/fileship.sh ~/bin/fileship.sh
    ```

## Use it

```sh
~/bin/fileship.sh                 # capture a region and upload it
~/bin/fileship.sh report.pdf a.png  # upload files
~/bin/fileship.sh --link https://example.com/long  # shorten a link
```

Without arguments, the crosshair of `screencapture -i` appears; press Escape to cancel. With files, each is uploaded and the links are copied together, separated by spaces. `--link` is there when your account can shorten links.

## Keyboard shortcut

1. Open the **Shortcuts** app and create a shortcut with one **Run Shell Script** action.
2. Enter the full path, for example `~/bin/fileship.sh`.
3. In the shortcut's details, add a keyboard shortcut.
4. Run it once. macOS asks to allow screen recording for Shortcuts (or the app that runs it); allow it in **System Settings → Privacy & Security → Screen & System Audio Recording**.

## Things to know

- The script sends each file in one request; your [reverse proxy](../proxy/overview.md#upload-size) must accept bodies as large as the file.
- The token is stored in the script in plain text. Keep it private, and download the script again after **Regenerate**.
- To change the options, download the script again.

> [!NOTE]
> Not tested yet on a Mac.
