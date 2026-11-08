---
title: Files
description: Upload, preview, search, organize and share files in the Fileship dashboard.
---

# Files

The **Files** page lists everything you uploaded. It needs the **Upload files** permission.

## Uploading

- **Upload** in the header (or press `U`) opens the upload sheet. Choose files, or switch to **Text** to create a text or code file from what you type.
- Drop files anywhere on the dashboard, or paste an image or file from the clipboard.
- You can keep working while files upload. The queue at the bottom shows their progress, and the link is copied to the clipboard when an upload finishes.

The **Options** apply to every file of the upload:

| Option          | What it does                                                                                     |
| --------------- | ------------------------------------------------------------------------------------------------ |
| **Folder**      | Put the files in a folder. Type a new name to create one.                                        |
| **File name**   | **Random** (8 characters), **UUID** or **Original**. This is the name in the link (`/u/<name>`). |
| **Expiration**  | Delete the file after 5 minutes up to 1 year, or never.                                          |
| **Compression** | Re-encode images as JPEG: none, low, medium, high or maximum.                                    |
| **Max views**   | Delete the file once it was viewed this many times. 0 means unlimited.                           |
| **Password**    | Visitors must enter it to open the file.                                                         |

Large files are sent in chunks (25 MB by default), several at a time, so uploads of any size work through a [reverse proxy](../proxy/overview.md#upload-size). Before uploading, Fileship warns you when the files do not fit in your storage limit (**Not enough space**) and when you already have a file with the same content (see [Duplicates](duplicates.md)).

**Original** names keep letters, digits, `-`, `_` and `.`; a name that is taken, reserved or starts with a dot is refused (**A file with that name already exists**). The display name of a file always keeps the original name with its Unicode characters, whatever the link name is.

## Finding files

- **Search files…** searches names, captions and the text in files. With AI on, **Smart search** finds images and videos by what they show and documents by what they say, in many languages (see [AI features](../ai.md#smart-search)).
- Filter by **Folder** and **File type**, and sort by date, name, size or views.
- Switch between **Grid view** and **List view**.
- Select several files to **Move**, **Download** or **Delete** them together.

## Previews

Opening a file shows a preview and its details:

- images (including animated GIF and WebP), videos and audio
- PDFs, page by page
- code, JSON, YAML and other text files with syntax highlighting, and Markdown rendered (up to 2 MB)
- for media, the resolution, duration, codec, bitrate and frame rate

**Details** shows the size, type, views (total and today), expiry and upload date. With AI on, **Insights** shows the caption and whether the file contains sensitive information.

## Editing a file

**Edit** changes the **Name**, the **Link name**, the **Folder**, the **Expiration**, the **Max views** and the **Password**. Changing the link name breaks the old link.

From a file's menu you can also **Copy link**, **Download**, move it to another folder, mark it as sensitive or not sensitive, and **Delete** it (to the [trash](trash.md)).

## Links

Every file has two addresses:

| Address             | What it shows                                                                                                                    |
| ------------------- | -------------------------------------------------------------------------------------------------------------------------------- |
| `/u/<link name>`    | The file itself, for embedding and downloading.                                                                                  |
| `/view/<link name>` | A page with the preview, the name and a download button, and the [link preview](../link-previews.md) for Discord and other apps. |

**Copy link** copies the `/u/` address, or the `/view/` page when embeds are on (and always for text files). Links use one of your [custom domains](../proxy/custom-domains.md) when you have some.

Visitors never see files in the trash, expired files or files marked as sensitive. Files that are uploaded as HTML, SVG, XML or JavaScript are downloaded instead of opened, so they cannot run in your domain. See [Sharing options](sharing.md).

## Uploading from elsewhere

- [ShareX](../uploaders/sharex.md), [macOS](../uploaders/macos.md) and [Linux](../uploaders/linux.md) scripts, [curl](../uploaders/curl.md)
- [Fileship for Windows](../desktop.md) and the [browser extension](../browser-extension.md)
- your [phone](../mobile.md)
- visitors, through your [inbox](inbox.md)
