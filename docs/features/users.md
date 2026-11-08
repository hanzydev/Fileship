---
title: Users and permissions
description: Roles and permissions in Fileship, storage and backup limits, impersonation, sessions and account security.
---

# Users and permissions

## Roles and permissions

Each account has a set of permissions. Admins manage them on **Administration → Users**.

| Permission        | Allows                                                                                                |
| ----------------- | ----------------------------------------------------------------------------------------------------- |
| **Upload files**  | Files, folders, the inbox, uploads from ShareX and the apps.                                          |
| **Take notes**    | Notes.                                                                                                |
| **Shorten links** | Short links.                                                                                          |
| **Admin**         | Everything above, plus the **Users**, **Logs** and **Statistics** pages. No storage or backup limits. |

A **Super admin** has every permission and also manages the instance: **Administration → Instance** (settings, storage, AI, imports), updates, clearing logs, and other super admins. Only super admins can create or edit super admins, and the last super admin cannot remove their own role. The account created at install (`admin`) is a super admin.

| Page or action                                                | Member | Admin | Super admin |
| ------------------------------------------------------------- | :----: | :---: | :---------: |
| Their own files, notes, links (with the permission)           |   ✓    |   ✓   |      ✓      |
| **Users**: create, edit, delete and invite members and admins |        |   ✓   |      ✓      |
| **Logs** and **Statistics**                                   |        |   ✓   |      ✓      |
| Impersonate users                                             |        |   ✓   |      ✓      |
| **Instance** settings and storage                             |        |       |      ✓      |
| Update Fileship, clear logs                                   |        |       |      ✓      |
| Create, edit and impersonate super admins                     |        |       |      ✓      |

## Managing users

On **Administration → Users**, **New user** creates an account, and each user's menu edits or deletes it. Editing sets:

- **Username** and **Password** (leave the password empty to keep it; a new password signs the user out everywhere)
- **Permissions**
- **Limits**: **Backups** and **Storage (MB)**, -1 for unlimited

Deleting a user deletes their files, folders, notes and short links too, immediately and without the trash. You cannot delete yourself or a super admin.

The page also shows each user's email address and whether it is confirmed, their storage use and **Pending invites**. See [Registration and invites](../configuration/registration.md).

## Impersonating

**Impersonate** in a user's menu logs you in as that user, to see what they see. A banner shows **You are signed in as …** with **Return to my account**. Admins cannot impersonate super admins. Impersonation is logged.

## Storage limits

A user with a storage limit cannot upload more once they reach it (**Storage limit reached**). Files in the trash count until they are deleted for good. Their usage shows in the sidebar and on **Settings → Storage**, and with [email](../configuration/email.md) set up they get a warning at 90%.

## Account security

Each user manages their own security in **Settings**:

- **Security**: password, **Authenticator app** (two-factor codes), **Passkeys** and [single sign-on](../configuration/single-sign-on.md) linking.
- **Sessions**: every signed-in device, with **Sign out** and **Sign out others**. Fileship for Windows shows up as its own session.
- **Integrations**: the upload token (see [Upload API](../api/uploads.md#upload-token)).

Sensitive actions (changing the password, regenerating the upload token, deleting backups, …) ask you to confirm it's you with your password, an authenticator code, a passkey or your single sign-on provider. Sessions do not expire by themselves; they end when you sign out or revoke them.
