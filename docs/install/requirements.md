---
title: Requirements
description: CPU, memory and disk Fileship needs for each AI preset, the supported processors and operating systems, and the browsers the dashboard supports.
---

# Requirements

## Hardware

|          | AI disabled | AI preset `low` | AI preset `medium` | AI preset `high` |
| -------- | ----------- | --------------- | ------------------ | ---------------- |
| **CPU**  | 2 cores     | 2 cores         | 4 cores            | 4 cores or more  |
| **RAM**  | 2 GB        | 4 GB            | 6 GB               | 8 GB             |
| **Disk** | 4 GB        | 6 GB            | 7 GB               | 7.5 GB           |

The disk sizes include about 1.8, 2.6 and 3 GB of AI models. Add the space for uploaded files kept on the local disk. AI runs on the CPU, so more cores analyze uploads faster; no GPU is needed. See [AI presets](../ai.md#ai-presets) for what each preset does, and turn AI off in **Administration → Instance → AI** on a small server.

Both x86-64 (amd64) and ARM64 are supported.

> [!NOTE]
> The ARM64 Docker images were tested under emulation only, not yet on real ARM64 hardware such as a Raspberry Pi 5.

## Software

| Install type                               | You need                                                                                     |
| ------------------------------------------ | -------------------------------------------------------------------------------------------- |
| [Docker](../installation.md) (recommended) | [Docker](https://www.docker.com/) with [Docker Compose](https://docs.docker.com/compose/) v2 |
| [Without Docker](without-docker.md)        | Node.js 24, pnpm 11, PostgreSQL 18, git and a C++ build toolchain                            |
| [Windows](windows.md)                      | Docker Desktop on WSL 2, or the same as without Docker                                       |

With Docker, PostgreSQL 18 is part of the stack and is kept up to date by the updater agent, including new major versions (see [Updating](../updating.md#postgresql)).

To serve Fileship on your own domain with HTTPS you also need a domain name and a [reverse proxy](../proxy/overview.md). The phone app and sharing to Fileship from Android need HTTPS (see [Fileship on your phone](../mobile.md)).

## Browsers

Fileship targets current versions of Chrome, Edge, Firefox, Brave and Zen Browser. Safari is supported through compatibility fallbacks, but a Chromium- or Firefox-based browser gives the best experience.

Some features need a secure context (HTTPS, or `http://localhost`): passkeys, checking for duplicates before an upload, installing the app and copying to the clipboard in some browsers.

## Apps

- [Fileship for Windows](../desktop.md): 64-bit Windows 11, version 24H2 (build 26100) or newer
- [Browser extension](../browser-extension.md): Chrome, Edge or another Chromium browser 116 or newer, or Firefox 142 or newer
- Both need a Fileship 6.0 server or newer.
