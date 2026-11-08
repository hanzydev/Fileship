---
title: Trash and retention
description: How long Fileship keeps deleted, expired and used-up items, and how scheduled backups are kept and rotated.
---

# Trash and retention

## Trash

Deleted files, folders and notes move to the trash for 30 days by default. Set **Trash retention (days)** in **Administration → Instance → Uploads** (or `NUXT_TRASH_RETENTION_DAYS`) to change it, from 0 to 365; `0` deletes everything immediately.

- Items in the trash still count toward a user's storage limit until they are deleted for good.
- Deleting a folder moves its subfolders to the trash with it; its files move to its parent unless **Also delete the files inside** is ticked. Restoring the folder restores what was trashed with it.
- Anything deleted from a shared folder goes to the trash of the folder's owner. Files that members of a shared folder upload belong to the owner and count toward the owner's storage limit.
- The delete confirmation has a **Delete forever** button that skips the trash.
- The trash is emptied of expired items every hour.

See [Trash](../features/trash.md) for how people use it.

## What is deleted immediately

Some items never go to the trash:

| What                                    | When                                                                      |
| --------------------------------------- | ------------------------------------------------------------------------- |
| Files with an expiry                    | When they expire. The check runs every 5 minutes (`DELETE_EXPIRED_CRON`). |
| Files with a view limit                 | When the response of the view that reaches the limit finishes.            |
| Short links with an expiry              | When they expire, with the same check.                                    |
| Short links with a click limit          | At the click that reaches the limit.                                      |
| Files and backups whose data is missing | At startup, when their storage no longer has them.                        |
| Everything of a deleted user            | When the user is deleted.                                                 |
| What a backup restore replaces          | During the restore.                                                       |

Unused invites, email confirmation and password reset links are removed when they expire.

## Scheduled backups

Users can back up their account automatically in **Settings → Backups**: every day or every week, at an hour in their own time zone, keeping the newest automatic backups up to the number they choose. Backups they create or upload themselves are never deleted automatically. A user's backup limit counts both kinds, so a scheduled backup is skipped while manual backups alone fill it. Due backups are checked every 15 minutes, and an instance that was offline makes up a missed backup once when it starts again.

Turn **Scheduled backups** off in **Administration → Instance → Uploads** (or set `NUXT_SCHEDULED_BACKUPS_ENABLED=false`) to stop them for everyone. Users' schedules are kept and continue when the setting is turned back on.

The backup limit of each user is set on the **Users** page (**Backups**, -1 for unlimited); admins have no limit. See [Backups](../features/backups.md).

## Logs

Log entries are kept until a super admin clears them on the **Logs** page. See [Logs](logs.md).
