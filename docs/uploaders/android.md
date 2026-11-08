---
title: Android
description: Upload photos, files, links and text from an Android phone to Fileship through the share sheet.
---

# Android

## Share to the installed app (recommended)

Install the Fileship dashboard as an app from Chrome, Edge or Samsung Internet, and **Fileship** appears in Android's share sheet. Shared files are uploaded with the same options as the upload sheet, links can be shortened or saved as notes, and text becomes a note. Your Fileship must be served over HTTPS.

See [Fileship on your phone](../mobile.md) for the steps.

## HTTP Shortcuts app

If you prefer an upload that runs without opening Fileship, the free [HTTP Shortcuts](https://http-shortcuts.rmy.ch/) app can send shared files to the [Upload API](../api/uploads.md):

1. Create a new **Regular Shortcut** named **Fileship**.
2. **Basic Request Settings**: method **POST**, URL `https://files.example.com/api/files`.
3. **Request Headers**: `Authorization` = `Bearer fs_...` (your token from **Settings → Integrations**).
4. **Request Body / Parameters**: **Form data (multipart)**. Add a parameter of type **File** named `file`, and turn on **Allow receiving files from the share sheet** in the shortcut's **Trigger & Execution** settings.
5. **Response Handling**: use a **Scripting** step after execution to copy the link:

    ```js
    copyToClipboard(JSON.parse(response.body).url);
    ```

The shortcut then shows up in the share sheet as **Fileship**.

> [!NOTE]
> The HTTP Shortcuts setup follows that app's documentation and has not been tested on a phone yet. The share target of the installed app was tested with a simulated share in a desktop browser only.

## Things to know

- Uploads through HTTP Shortcuts are single requests with the whole file; your [reverse proxy](../proxy/overview.md#upload-size) must accept bodies that large. The installed app uploads in chunks.
- Keep the token private and update the shortcut after **Regenerate**.
