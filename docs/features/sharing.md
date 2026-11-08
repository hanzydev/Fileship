---
title: Sharing options
description: Control who can open what you share from Fileship - passwords, expiry, view and click limits, sensitive files, public folders and notes.
---

# Sharing options

Everything in Fileship has a link. These options decide who can open it and for how long.

## Files

| Option                | Set it                                  | Effect                                                                                                                             |
| --------------------- | --------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| **Password**          | upload options, or **Edit** on the file | Visitors must enter it on the file's page. Up to 48 characters. You can see it in the file's details.                              |
| **Expiration**        | upload options, or **Edit**             | The file is deleted for good after 5 minutes up to 1 year.                                                                         |
| **Max views**         | upload options, or **Edit**             | The file is deleted once it was viewed this many times. 0 means unlimited.                                                         |
| **Mark as sensitive** | the file's menu                         | The file is hidden from visitors and public folders. AI can also mark files (see [Sensitive content](../ai.md#sensitive-content)). |

How views are counted:

- A visitor's view counts once per IP address, browser and file every 10 minutes. Your own views and those of people you share a folder with are not counted.
- The view that reaches **Max views** is still served; the file is deleted when it finishes.
- Visitors get a "not found" page for expired files, sensitive files and files in the trash.

## Folders and notes

- A folder is private until you turn on **Public**. See [Public folders](folders.md#public-folders).
- A note is private until you choose **Make public**. See [Notes](notes.md).
- [Team folders](folders.md#team-folders) share a folder with specific users instead of everyone.

## Short links

Short links can have a **Password**, **Max clicks** and an **Expiration**. See [Short links](short-links.md).

## How links look

- [Link previews](../link-previews.md) set the title, description, color and Discord layout people see when you paste a link in a chat.
- [Custom domains](../proxy/custom-domains.md) put your links on other domains.

## What visitors never get

Uploaded HTML, SVG, XML and JavaScript files are downloaded instead of shown, and every file is served with headers that stop it from running scripts on your domain. PDFs are drawn by Fileship's own viewer without scripts. Visitors never see who uploaded a file, its internal details or files in private folders.
