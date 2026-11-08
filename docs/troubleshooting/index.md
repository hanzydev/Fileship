---
title: Troubleshooting
description: Find the fix for a Fileship problem by symptom, or look up any error message the dashboard or API shows.
---

# Troubleshooting

## By topic

| Page                                                  | For example                                                                      |
| ----------------------------------------------------- | -------------------------------------------------------------------------------- |
| [Installation and startup](install-and-startup.md)    | A container keeps restarting, the health check stays starting, lost `secret.key` |
| [Uploads](uploads.md)                                 | 413 from the proxy, Storage limit reached, Invalid upload chunk                  |
| [Login and security](login-and-security.md)           | Too many attempts, everyone has the same IP, Invalid origin, passkeys            |
| [Single sign-on](single-sign-on.md)                   | Wrong address, discovery fails, Google `invalid_client`, no linked account       |
| [Live updates](realtime.md)                           | Lists only change after a reload                                                 |
| [Email](email.md)                                     | Couldn't send the test email                                                     |
| [AI](ai.md)                                           | Model downloads, out of memory, a stuck queue                                    |
| [Storage](storage.md)                                 | Could not connect, Could not list the storage                                    |
| [Webhooks](webhooks.md)                               | Private addresses, timeouts, webhooks that turned off                            |
| [Updates](updates.md)                                 | Couldn't update Fileship, the updater can't be reached                           |
| [Windows app and extension](desktop-and-extension.md) | Server too old, invalid upload token, missing permissions                        |

Before anything else, read the server output: `docker compose logs --tail=200 fileship-blue fileship-green` with Docker, `journalctl -u fileship` without.

## Error messages

Every message the dashboard and the API show, in English, and where to read about it. `…` stands for a value filled in, such as a name, a number or an error from another system. Messages also appear in the language set for the account; the API answers in the language of the `Accept-Language` header.

| Message                                                                                                         | Read                                                                                               |
| --------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| A backup is being restored, try again when it finishes                                                          | [Backups](../features/backups.md#restoring)                                                        |
| A backup restoration is already in progress                                                                     | [Backups](../features/backups.md)                                                                  |
| A Discord layout is empty or too large for Discord                                                              | [Link previews](../link-previews.md)                                                               |
| A file with that name already exists                                                                            | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| A file with that name is in the trash                                                                           | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| A folder can't be moved into itself or a folder inside it                                                       | [Folders](../features/folders.md)                                                                  |
| A migration is already running                                                                                  | [Storage](storage.md#a-migration-is-already-running--wait-for-the-import-to-finish)                |
| A password is required                                                                                          | [Inbox](../features/inbox.md)                                                                      |
| A user with that username already exists                                                                        | [Users and permissions](../features/users.md#managing-users)                                       |
| AI features are turned off                                                                                      | [AI](ai.md#ai-features-are-turned-off)                                                             |
| An archive job is already running for this folder                                                               | [Folders](../features/folders.md)                                                                  |
| An import is already running                                                                                    | [Storage](storage.md#a-migration-is-already-running--wait-for-the-import-to-finish)                |
| Another account already uses this email address                                                                 | [Email setup](../configuration/email.md#email-addresses)                                           |
| Archive not found                                                                                               | [Folders](../features/folders.md)                                                                  |
| Avatar must be at least 64x64 pixels                                                                            | [Instance settings](../configuration/instance-settings.md#uploads)                                 |
| Avatar must be less than … MB                                                                                   | [Instance settings](../configuration/instance-settings.md#uploads)                                 |
| Backup limit must be a whole number                                                                             | [Backups](../features/backups.md)                                                                  |
| Backup limit must be at least -1                                                                                | [Backups](../features/backups.md)                                                                  |
| Backup limit reached                                                                                            | [Backups](../features/backups.md)                                                                  |
| Backup not found                                                                                                | [Backups](../features/backups.md#restoring)                                                        |
| Backups must be .tgz files                                                                                      | [Backups](../features/backups.md)                                                                  |
| Choose at least one event                                                                                       | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| Chunk offset must be at least 0                                                                                 | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Color must be 7 characters                                                                                      | [Link previews](../link-previews.md)                                                               |
| Color must start with #                                                                                         | [Link previews](../link-previews.md)                                                               |
| Compression must be at least 0                                                                                  | [Files](../features/files.md)                                                                      |
| Compression must be at most 100                                                                                 | [Files](../features/files.md)                                                                      |
| Content must be at least 1 character                                                                            | [Notes](../features/notes.md)                                                                      |
| Content must be at most 50000 characters                                                                        | [Notes](../features/notes.md)                                                                      |
| Could not connect: …                                                                                            | [Storage](storage.md#could-not-connect-)                                                           |
| Could not list the storage: …                                                                                   | [Storage](storage.md#could-not-list-the-storage-)                                                  |
| Could not read the image                                                                                        | [Windows app and extension](desktop-and-extension.md#text-capture-says-ai-features-are-turned-off) |
| Couldn't log in with …. Try again.                                                                              | [Single sign-on](single-sign-on.md)                                                                |
| Couldn't read the discovery document: …                                                                         | [Single sign-on](single-sign-on.md#couldnt-read-the-discovery-document)                            |
| Couldn't send the email. Try again later.                                                                       | [Email](email.md#couldnt-send-the-email-try-again-later)                                           |
| Couldn't send the test email: …                                                                                 | [Email](email.md#couldnt-send-the-test-email-)                                                     |
| Current chunk must be at least 1                                                                                | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Domains must be at most 253 characters                                                                          | [Custom domains](../proxy/custom-domains.md)                                                       |
| Email isn't set up on this instance                                                                             | [Email](email.md#email-isnt-set-up-on-this-instance)                                               |
| Enter a name                                                                                                    | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| Enter a username                                                                                                | [Folders](../features/folders.md)                                                                  |
| Enter a valid http or https URL                                                                                 | [Short links](../features/short-links.md)                                                          |
| Enter your two-factor authentication code                                                                       | [Login and security](login-and-security.md#two-factor-authentication)                              |
| Expiration must be at least 0                                                                                   | [Short links](../features/short-links.md)                                                          |
| Failed to process avatar                                                                                        | [Instance settings](../configuration/instance-settings.md#uploads)                                 |
| File name cannot start with a dot                                                                               | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| File name must be at least 3 characters                                                                         | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| File name must be at most 255 characters                                                                        | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| File not found                                                                                                  | [Files](../features/files.md)                                                                      |
| Finish logging in with single sign-on first                                                                     | [Single sign-on](single-sign-on.md)                                                                |
| Folder not found                                                                                                | [Folders](../features/folders.md)                                                                  |
| Folders can be nested up to … levels deep                                                                       | [Folders](../features/folders.md)                                                                  |
| Inbox not found                                                                                                 | [Inbox](../features/inbox.md)                                                                      |
| Inbox password must be at most 48 characters                                                                    | [Inbox](../features/inbox.md)                                                                      |
| Invalid authentication response                                                                                 | [Login and security](login-and-security.md#passkeys-dont-work)                                     |
| Invalid avatar                                                                                                  | [Instance settings](../configuration/instance-settings.md#uploads)                                 |
| Invalid avatar image                                                                                            | [Instance settings](../configuration/instance-settings.md#uploads)                                 |
| Invalid backup file                                                                                             | [Backups](../features/backups.md)                                                                  |
| Invalid file                                                                                                    | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Invalid files                                                                                                   | [Folders](../features/folders.md)                                                                  |
| Invalid origin                                                                                                  | [Login and security](login-and-security.md#invalid-origin)                                         |
| Invalid passkey                                                                                                 | [Login and security](login-and-security.md#passkeys-dont-work)                                     |
| Invalid password                                                                                                | [Login and security](login-and-security.md)                                                        |
| Invalid query                                                                                                   | [Files](../features/files.md)                                                                      |
| Invalid two-factor authentication code                                                                          | [Login and security](login-and-security.md#two-factor-authentication)                              |
| Invalid upload chunk                                                                                            | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Invalid upload id                                                                                               | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Invalid username or password                                                                                    | [Login and security](login-and-security.md#too-many-attempts-try-again-later)                      |
| Invalid verification method                                                                                     | [Login and security](login-and-security.md)                                                        |
| Invite not found                                                                                                | [Email](email.md#this-link-is-invalid-or-has-expired)                                              |
| Job is not done yet                                                                                             | [Folders](../features/folders.md)                                                                  |
| Job not found                                                                                                   | [Folders](../features/folders.md)                                                                  |
| Link not found                                                                                                  | [Short links](../features/short-links.md)                                                          |
| Max clicks must be at least 0                                                                                   | [Short links](../features/short-links.md)                                                          |
| Max views must be at least 0                                                                                    | [Files](../features/files.md)                                                                      |
| Missing backup file                                                                                             | [Backups](../features/backups.md)                                                                  |
| Missing file                                                                                                    | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Missing image                                                                                                   | [Windows app and extension](desktop-and-extension.md#text-capture-says-ai-features-are-turned-off) |
| Name can be at most 64 characters                                                                               | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| Name is required                                                                                                | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| Name must be at least 1 character                                                                               | [Folders](../features/folders.md)                                                                  |
| Name must be at least 3 characters                                                                              | [Users and permissions](../features/users.md#managing-users)                                       |
| Name must be at most 255 characters                                                                             | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| Name must be at most 32 characters                                                                              | [Users and permissions](../features/users.md#managing-users)                                       |
| No account is linked to this … account. Ask an admin, or log in another way and link it in Settings → Security. | [Single sign-on](single-sign-on.md#no-account-is-linked-to-this-account)                           |
| No files found to delete                                                                                        | [Files](../features/files.md)                                                                      |
| No files to archive in this folder                                                                              | [Folders](../features/folders.md)                                                                  |
| No single sign-on account is linked                                                                             | [Single sign-on](single-sign-on.md#no-account-is-linked-to-this-account)                           |
| No user has that username                                                                                       | [Folders](../features/folders.md)                                                                  |
| Note not found                                                                                                  | [Notes](../features/notes.md)                                                                      |
| Nothing to update                                                                                               | [Updates](updates.md)                                                                              |
| Open Fileship at its dashboard address to use single sign-on.                                                   | [Single sign-on](single-sign-on.md#open-fileship-at-its-dashboard-address-to-use-single-sign-on)   |
| Open Fileship at … to link single sign-on                                                                       | [Single sign-on](single-sign-on.md#open-fileship-at-its-dashboard-address-to-use-single-sign-on)   |
| Passkey not found                                                                                               | [Login and security](login-and-security.md#passkeys-dont-work)                                     |
| Password login is turned off, log in with single sign-on                                                        | [Single sign-on](single-sign-on.md)                                                                |
| Password login is turned off, so you couldn't log in without single sign-on                                     | [Single sign-on](single-sign-on.md#set-a-password-or-add-a-passkey-first)                          |
| Password must be at least 8 characters                                                                          | [Users and permissions](../features/users.md#managing-users)                                       |
| Password must be at most 48 characters                                                                          | [Users and permissions](../features/users.md#managing-users)                                       |
| Permissions must be provided if user is not a super admin                                                       | [Users and permissions](../features/users.md#managing-users)                                       |
| Registration is disabled                                                                                        | [Registration](../configuration/registration.md#self-registration)                                 |
| Secret can be at most 256 characters                                                                            | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| Session not found                                                                                               | [Login and security](login-and-security.md)                                                        |
| Set a password or add a passkey first, so you can still log in                                                  | [Single sign-on](single-sign-on.md#set-a-password-or-add-a-passkey-first)                          |
| Single sign-on isn't available right now                                                                        | [Single sign-on](single-sign-on.md#single-sign-on-isnt-available-right-now)                        |
| Single sign-on isn't available right now. Try again later or ask an admin.                                      | [Single sign-on](single-sign-on.md#single-sign-on-isnt-available-right-now)                        |
| Slug already taken                                                                                              | [Inbox](../features/inbox.md)                                                                      |
| Slug can only contain letters, numbers, dashes and underscores                                                  | [Inbox](../features/inbox.md)                                                                      |
| Slug cannot be empty                                                                                            | [Inbox](../features/inbox.md)                                                                      |
| Slug must be at most 48 characters                                                                              | [Inbox](../features/inbox.md)                                                                      |
| Some fields are invalid                                                                                         | [Upload API](../api/uploads.md#errors)                                                             |
| Storage is in use                                                                                               | [Storage](storage.md#storage-is-in-use)                                                            |
| Storage limit reached                                                                                           | [Uploads](uploads.md#storage-limit-reached)                                                        |
| Storage not found                                                                                               | [Storage setup](../configuration/storage.md)                                                       |
| That short link is already taken                                                                                | [Short links](../features/short-links.md)                                                          |
| The account with this email address is already linked to a different … account.                                 | [Single sign-on](single-sign-on.md#no-account-is-linked-to-this-account)                           |
| The authenticator app is already disabled                                                                       | [Login and security](login-and-security.md#two-factor-authentication)                              |
| The authenticator app is already enabled                                                                        | [Login and security](login-and-security.md#two-factor-authentication)                              |
| The chosen file is gone, choose it again                                                                        | [Moving from another app](../migrating.md)                                                         |
| The code must be 6 digits                                                                                       | [Login and security](login-and-security.md#two-factor-authentication)                              |
| The code must contain only digits                                                                               | [Login and security](login-and-security.md#two-factor-authentication)                              |
| The image is too large                                                                                          | [Windows app and extension](desktop-and-extension.md#text-capture-says-ai-features-are-turned-off) |
| The inbox folder and the folders that contain it can't be deleted                                               | [Folders](../features/folders.md#inbox-folder)                                                     |
| The login took too long or was started in another window. Try again.                                            | [Single sign-on](single-sign-on.md#the-login-took-too-long-or-was-started-in-another-window)       |
| The login with … was cancelled.                                                                                 | [Single sign-on](single-sign-on.md)                                                                |
| The request expired, try again                                                                                  | [Login and security](login-and-security.md)                                                        |
| The request is too large                                                                                        | [Uploads](uploads.md#the-request-is-too-large)                                                     |
| The request must include its length                                                                             | [Uploads](uploads.md#the-request-must-include-its-length)                                          |
| The request was interrupted                                                                                     | [Uploads](uploads.md#uploads-stop-halfway-or-time-out)                                             |
| The security check failed, try again                                                                            | [Login and security](login-and-security.md#locked-out-by-turnstile)                                |
| This file name is reserved                                                                                      | [Uploads](uploads.md#a-file-with-that-name-already-exists)                                         |
| This invitation is invalid or has expired                                                                       | [Email](email.md#this-link-is-invalid-or-has-expired)                                              |
| This isn't a Zipline export or an XBackBone or Chibisafe database                                               | [Moving from another app](../migrating.md)                                                         |
| This link is invalid or has expired                                                                             | [Email](email.md#this-link-is-invalid-or-has-expired)                                              |
| This passkey isn't registered here                                                                              | [Login and security](login-and-security.md#passkeys-dont-work)                                     |
| This person has no access to this folder                                                                        | [Folders](../features/folders.md)                                                                  |
| This slug is reserved                                                                                           | [Inbox](../features/inbox.md)                                                                      |
| This URL points to a private or local address. A super admin can allow these in the instance settings.          | [Webhooks](webhooks.md#this-url-points-to-a-private-or-local-address)                              |
| This username is already taken                                                                                  | [Users and permissions](../features/users.md#managing-users)                                       |
| This … account is already linked to another account.                                                            | [Single sign-on](single-sign-on.md#no-account-is-linked-to-this-account)                           |
| Thumbnail not found                                                                                             | [Files](../features/files.md)                                                                      |
| Title must be at least 3 characters                                                                             | [Notes](../features/notes.md)                                                                      |
| Title must be at most 48 characters                                                                             | [Notes](../features/notes.md)                                                                      |
| Too many attempts, try again later                                                                              | [Login and security](login-and-security.md#too-many-attempts-try-again-later)                      |
| Too many chunks                                                                                                 | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Total chunks must be at least 1                                                                                 | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Two-factor authentication is enabled, enter the code from your authenticator app                                | [Login and security](login-and-security.md#two-factor-authentication)                              |
| Two-factor authentication is not enabled for this account                                                       | [Login and security](login-and-security.md#two-factor-authentication)                              |
| Upload already completed                                                                                        | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| Upload not found                                                                                                | [Uploads](uploads.md#invalid-upload-chunk)                                                         |
| URL can be at most 2048 characters                                                                              | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| URL must be at most 2048 characters                                                                             | [Short links](../features/short-links.md)                                                          |
| Usable space must be a whole number                                                                             | [Users and permissions](../features/users.md#managing-users)                                       |
| Usable space must be at least -1                                                                                | [Users and permissions](../features/users.md#managing-users)                                       |
| Use a secret of at least 16 characters                                                                          | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| Use the … account that is linked to your account.                                                               | [Single sign-on](single-sign-on.md)                                                                |
| Use up to 32 letters, numbers, dashes or underscores                                                            | [Short links](../features/short-links.md)                                                          |
| User not found                                                                                                  | [Users and permissions](../features/users.md)                                                      |
| Username already taken                                                                                          | [Users and permissions](../features/users.md#managing-users)                                       |
| Username must be at least 3 characters                                                                          | [Users and permissions](../features/users.md#managing-users)                                       |
| Username must be at most 24 characters                                                                          | [Users and permissions](../features/users.md#managing-users)                                       |
| Verification is required                                                                                        | [Login and security](login-and-security.md)                                                        |
| Wait for the import to finish                                                                                   | [Storage](storage.md#a-migration-is-already-running--wait-for-the-import-to-finish)                |
| Wait for the storage migration to finish                                                                        | [Storage](storage.md#a-migration-is-already-running--wait-for-the-import-to-finish)                |
| Webhook not found                                                                                               | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| You are already logged in                                                                                       | [Login and security](login-and-security.md)                                                        |
| You are not signed in as another user                                                                           | [Users and permissions](../features/users.md#impersonating)                                        |
| You can add at most 50 domains                                                                                  | [Custom domains](../proxy/custom-domains.md)                                                       |
| You can add up to … webhooks                                                                                    | [Webhooks](webhooks.md#use-a-secret-of-at-least-16-characters--you-can-add-up-to-10-webhooks)      |
| You can keep up to … backups                                                                                    | [Backups](../features/backups.md)                                                                  |
| You can't share a folder with yourself                                                                          | [Folders](../features/folders.md)                                                                  |
| You cannot delete a super admin                                                                                 | [Users and permissions](../features/users.md#managing-users)                                       |
| You cannot delete yourself                                                                                      | [Users and permissions](../features/users.md#managing-users)                                       |
| You cannot grant super admin rights                                                                             | [Users and permissions](../features/users.md#managing-users)                                       |
| You cannot remove super admin from yourself if you are the only super admin                                     | [Users and permissions](../features/users.md#managing-users)                                       |
| You cannot sign in as this user                                                                                 | [Users and permissions](../features/users.md#impersonating)                                        |
| You cannot update a super admin                                                                                 | [Users and permissions](../features/users.md#managing-users)                                       |
| You do not have permission to access this page                                                                  | [Notes](../features/notes.md)                                                                      |
| You do not have permission to create a super admin                                                              | [Users and permissions](../features/users.md#managing-users)                                       |
| You do not have permission to perform this action                                                               | [Upload API](../api/uploads.md#errors)                                                             |
| You must be logged in to perform this action                                                                    | [Upload API](../api/uploads.md#errors)                                                             |
| … is not a valid domain                                                                                         | [Custom domains](../proxy/custom-domains.md)                                                       |

Messages about a field's length or format (**Must be at least … characters**, **Enter a valid URL**, **Choose at least …**) come from the form you filled in and say what to change.
