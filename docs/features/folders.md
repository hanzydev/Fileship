---
title: Folders and team folders
description: Group files in nested folders, share them publicly with one link, download them as an archive and work on them together with viewers and editors.
---

# Folders and team folders

The **Folders** page lists your folders. Folders need the **Upload files** permission.

## Creating and nesting folders

**New folder** creates a folder with a **Name** (up to 32 characters). Open a folder to see its files and the folders inside it; folders can be nested up to 10 levels deep. **Move** in a folder's menu puts it inside another folder or back at the **Top level**, with everything in it.

To fill a folder, upload into it (the upload sheet's **Folder** option, or upload while the folder is open), use **Move** on files, or **Manage files** on the folder page to pick files from your library.

## Public folders

Turn on **Public** to let anyone with the link view the folder's files. **Copy public link** copies its address (`/folder/<id>`).

- Visitors see the public folders inside a public folder, but not the private ones.
- Password protected files are listed as locked; visitors need the password to open them.
- Sensitive files, expired files and files in the trash are never shown to visitors. While AI is on, images and text files only appear once they have been checked.
- **Download as archive** downloads the folder, with the folders inside it, as a `.tar.gz` archive. Visitors' archives leave out locked and sensitive files.

## Team folders

**Share** in a folder's menu gives other users access by **Username**, as a **Viewer** or an **Editor**. Access covers the folder and everything inside it, and the strongest role on the way down wins. Shared folders appear under **Shared with me** on the **Folders** page.

| Action                                                               | Viewer | Editor | Owner |
| -------------------------------------------------------------------- | :----: | :----: | :---: |
| Open, preview and download files, download as archive                |   ✓    |   ✓    |   ✓   |
| Upload files and create folders                                      |        |   ✓    |   ✓   |
| Rename files and folders                                             |        |   ✓    |   ✓   |
| Move files and folders to the trash                                  |        |   ✓    |   ✓   |
| Move files and folders                                               |        |        |   ✓   |
| Share the folder, change roles, make it public or private            |        |        |   ✓   |
| Change file settings (link name, password, expiry, views, sensitive) |        |        |   ✓   |
| Delete forever                                                       |        |        |   ✓   |

- Everything in a shared folder belongs to its owner. Files an editor uploads count toward the owner's storage limit, use the owner's AI settings and go to the owner's trash when deleted.
- An editor cannot delete the shared folder itself, only what is inside it.
- **People with access** lists the members. The owner can change a role or **Remove access**; a member can **Leave folder**.
- Members need the **Upload files** permission to open folder pages.
- Shared folders are not part of account backups, and do not appear in [Fileship Harbor](../desktop.md).

## Inbox folder

If you use the [inbox](inbox.md), files people send you land in a folder called **Inbox**. It and the folders that contain it cannot be deleted.

## Deleting folders

Deleting a folder moves it to the [trash](trash.md) together with the folders inside it. Its files move to the folder's parent, unless you tick **Also delete the files inside**, which trashes them with it. Restoring the folder brings back what was deleted with it.
