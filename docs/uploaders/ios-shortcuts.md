---
title: iOS Shortcuts
description: Upload photos and files from an iPhone or iPad to Fileship from the share sheet, with a shortcut built in the Shortcuts app.
---

# iOS Shortcuts

Safari cannot add web apps to the iOS share sheet, so the [installed Fileship app](../mobile.md) does not appear there. A shortcut in Apple's **Shortcuts** app can upload to Fileship from the share sheet instead.

> [!NOTE]
> This shortcut was written against the [Upload API](../api/uploads.md) and has not been tested on an iPhone yet.

## Build the shortcut

You need your upload token from **Settings → Integrations** in the dashboard.

1. Open **Shortcuts**, tap **+** and name the shortcut **Upload to Fileship**.
2. Tap the **ⓘ** button and turn on **Show in Share Sheet**. Under **Share Sheet Types**, keep **Images**, **Media** and **Files**.
3. Add **Get Contents of URL** and set:
    - **URL**: `https://files.example.com/api/files`
    - **Method**: **POST**
    - **Headers**: add `Authorization` with the value `Bearer fs_...` (your token)
    - **Request Body**: **Form**. Add a field of type **File** with the key `file` and the value **Shortcut Input**. Optionally add text fields such as `fileNameType` = `Random`.
4. Add **Get Dictionary Value**: get **Value** for the key `url` in **Contents of URL**.
5. Add **Copy to Clipboard** with that value.
6. Optionally add **Show Notification** with the text `Link copied`.

## Use it

In Photos, Files or any app, tap **Share** and choose **Upload to Fileship**. The link is copied to the clipboard when the upload finishes.

## Shorten links

Make a second shortcut that accepts **URLs** from the share sheet, with **Get Contents of URL** set to `https://files.example.com/api/links`, method **POST**, the same `Authorization` header and a **JSON** body with the key `url` set to **Shortcut Input**. Get the value for `shortUrl` and copy it.

## Things to know

- Each upload is one request with the whole file; your [reverse proxy](../proxy/overview.md#upload-size) must accept bodies that large. Videos from an iPhone can be several hundred megabytes.
- Your token is stored inside the shortcut. Don't share the shortcut with others, and update it after **Regenerate**.
