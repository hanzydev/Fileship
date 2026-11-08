---
title: Backups
description: Back up your Fileship account by hand or on a schedule, download the archive, and restore it later or on another server.
---

# Backups

**Settings → Backups** exports everything in your account to one archive: your files, folders, notes and short links. Use it to keep a copy, or to move your account to another Fileship server.

> [!NOTE]
> These are per-account backups. To back up the whole server, back up its database and data folder; see [Backing up the database](../backups/database.md).

## Manual backups

- **Create** makes a backup in the background. It appears in the list when it is ready.
- **Download** saves the archive (`.tgz`).
- **Upload** adds a backup archive from your computer, for example one made on another server. You can then restore it.
- **Delete** removes a backup.

## Automatic backups

Under **Automatic backups**, choose a **Schedule** (**Off**, **Every day** or **Every week**), the **Day** and **Time** in your time zone, and how many **Backups to keep**. After each new automatic backup, the oldest automatic ones beyond that number are deleted. Backups you create or upload yourself are never deleted automatically.

Your account's backup limit counts both kinds. When manual backups alone fill it, a scheduled backup is skipped (**Skipped … because the backup limit is reached**). **Next backup** and **Last backup** show what happens next and what happened last.

If the admin turned scheduled backups off for the server, the page says so; your schedule is kept and starts again when they turn them back on.

## Restoring

**Restore** replaces your current data with the backup: your current files, folders, notes and short links are deleted first. Restoring runs in the background and can take a while for large accounts; while it runs, the server does not update itself.

- Files that do not fit in your storage limit make the restore fail before anything is changed.
- Restored items always belong to you, even when the backup came from another account.
- Not restored: who you shared folders with and linked single sign-on accounts.

## Limits

Each user's backup limit is set by an admin on the **Users** page (**Backups**, -1 for unlimited). Admins have no limit. Backups are stored on the active [storage](../configuration/storage.md).
