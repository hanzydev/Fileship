---
title: Flameshot
description: Use Flameshot's capture and annotation tools and upload the result to Fileship.
---

# Flameshot

[Flameshot](https://flameshot.org/) captures a region and lets you annotate it before saving. Its built-in upload button only supports Imgur, so send the result to Fileship through the [Linux script](linux.md) instead.

## With the Linux script

The Linux script already uses Flameshot when `grim`/`slurp` are not installed: it runs `flameshot gui --raw`, so you get Flameshot's annotation tools, and uploads the image when you confirm with Enter or Ctrl+C.

1. Install Flameshot and the script as described in [Linux](linux.md).
2. Bind `~/.local/bin/fileship` to a key.

If you also have `grim` and `slurp` installed and prefer Flameshot, change the order in the script's capture block, or call Flameshot yourself:

```sh
flameshot gui --raw > /tmp/shot.png && fileship /tmp/shot.png
```

## On Windows and macOS

Flameshot runs there too, but has no shell script around it. Save the capture and upload it with [curl](curl.md), or use [ShareX](sharex.md) or [Fileship for Windows](../desktop.md) on Windows and the [macOS script](macos.md) on a Mac.

> [!NOTE]
> Not tested with Flameshot itself; the commands follow Flameshot's documented `--raw` option.
