---
title: ShareX
description: Upload screenshots and files from ShareX on Windows to Fileship, and shorten links with it, using the configs from the Integrations page.
---

# ShareX

[ShareX](https://getsharex.com/) can upload to Fileship as a custom uploader. Fileship writes the config for you.

Prefer an app made for Fileship? [Fileship for Windows](../desktop.md) captures, records and uploads ShareX style, and adds Fileship Harbor to File Explorer.

## File uploader

1. In the dashboard, open **Settings → Integrations**.
2. Under **Upload options**, choose the defaults for everything you upload from ShareX: **File name** (**Random**, **UUID** or **Original**), **Expiration**, **Compression**, **Max views**, **Password** and **Folder**.
3. On the **ShareX** card, choose **Download**. You get `fileship.sxcu`.
4. Open the file. ShareX imports it as a custom uploader.
5. In ShareX, set **Destinations → Image uploader**, **Text uploader** and **File uploader** to **Custom uploader** (and pick the Fileship one under **Destinations → Custom uploader settings** if you have several).

The config posts to `https://<your Fileship>/api/files` with your upload token and copies the returned `url` to the clipboard. It uses the address you downloaded it from, so download it from your public address, not `localhost`.

To change the options later, download and import the config again.

## URL shortener

With the **Shorten links** permission, the **ShareX URL shortener** card downloads `fileship-shortener.sxcu`. Open it, then set **Destinations → URL shortener** to **Custom URL shortener**. ShareX then shortens links with your Fileship short links and copies the `shortUrl`.

## What the config contains

```json
{
    "Version": "16.1.0",
    "Name": "Fileship - File Uploader",
    "DestinationType": "ImageUploader, TextUploader, FileUploader",
    "RequestMethod": "POST",
    "RequestURL": "https://files.example.com/api/files",
    "Headers": { "Authorization": "Bearer fs_..." },
    "URL": "{json:url}",
    "ErrorMessage": "{json:message}",
    "Body": "MultipartFormData",
    "FileFormName": "file",
    "Arguments": { "fileNameType": "Random", "maxViews": 0, "compression": 0 }
}
```

`Arguments` holds the upload options you chose. See the [Upload API](../api/uploads.md) for every field.

## Things to know

- ShareX sends each file in **one request**, not in chunks. Your [reverse proxy](../proxy/overview.md#upload-size) must accept bodies as large as the files you upload (Nginx's default is 1 MB).
- **Regenerate** on the Integrations page invalidates the token in every config you downloaded; download and import them again.
- Errors show in ShareX with Fileship's message, for example **Storage limit reached**.

> [!NOTE]
> Not tested yet with ShareX itself; the config follows ShareX's custom uploader format.
