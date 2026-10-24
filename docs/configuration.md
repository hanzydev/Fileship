# Configuration

## Instance settings

Super admins can configure Fileship from the dashboard in **Administration → Instance**. The changes apply immediately, without a restart:

- **General:** site name and link protocol
- **Uploads:** chunk size, avatar size limit, trash retention, EXIF removal and scheduled backups
- **Registration:** self-registration, default permissions (upload files, take notes, shorten links) and storage limit
- **Turnstile:** site and secret keys
- **Email:** dashboard address, SMTP server, encryption, credentials and sender address, plus **Send test email**
- **AI:** features on or off, preset, parallel jobs, threads per job and job heartbeat timeout, plus the analysis queue with **Retry failed** and **Reset AI data**
- **Logs and telemetry:** log level and anonymous telemetry
- **Storage:** where uploads and backups are kept

Each of these settings except Storage can also be given a default through an environment variable (see [`apps/dashboard/.env.example`](../apps/dashboard/.env.example)). A value outside the allowed range falls back to the built-in default. A value saved in the dashboard always overrides the environment, and every field can be reset to its default.

## Storage

Fileship keeps uploads and backups in its data directory by default. In **Administration → Instance → Storage** you can add S3 compatible storage (AWS S3, Cloudflare R2, MinIO, Backblaze B2 and others), SFTP or FTP, and choose which one new files go to. Each file stays where it was written, so switching storage does not move anything. Use **Move everything here** on the active storage to move existing files and backups to it.

Uploads stream to the storage chunk by chunk. Thumbnails and temporary files always stay in the data directory. Storage credentials are encrypted with `data/secret.key`, so keep that file together with your data.

To find duplicates (**Settings → Storage → Duplicates**), Fileship reads a file back once and stores its SHA-256 hash, but only when another file of the same user has exactly the same size. On S3, SFTP and FTP this read counts as a download from the storage.

On every start, Fileship removes files and backups whose data is missing from their storage. A storage that cannot be reached or looks empty is skipped, so a misconfigured storage never deletes anything.

## Email

Fileship can send email: links to confirm an email address or reset a password, invites and storage warnings. Set it up in **Administration → Instance → Email**, or give the defaults with `NUXT_SITE_URL` and the `NUXT_SMTP_*` variables:

- **Dashboard address:** the address people open Fileship at, for example `https://files.example.com`. Links in emails always use it, never the address a request came from.
- **SMTP server, port and encryption:** STARTTLS (usually port 587), TLS (usually port 465) or none.
- **Username and password:** leave them empty if the server accepts mail without signing in. The password is encrypted with `data/secret.key` and never shown again; leave the field empty to keep it.
- **Sender address:** emails come from this address, with the site name as the sender name.

**Send test email** uses the values in the form, so you can try them before saving. Until the dashboard address, the SMTP server and the sender address are set, every email feature stays hidden.

Once email works, people can add an email address in **Settings → Profile**. Fileship sends a confirmation link that is valid for 24 hours and only uses the address after it is confirmed. An address can belong to one account only. Admins see each user's address, and whether it is confirmed, on the **Users** page.

**Forgot your password?** on the login page sends a reset link to the account's confirmed address. The link works once, for one hour, and resetting the password signs the account out everywhere. Accounts that use an authenticator app must enter a code too, so access to the mailbox alone is never enough. The page answers the same way whether or not an account matches, so it never reveals which accounts exist. Without a confirmed address, an admin can still set a new password on the **Users** page.

Admins can invite people from the **Users** page: **Invite** sends a link to create an account with the permissions and storage limit you choose, in the language you pick. The link works once, for seven days, even while self-registration is off, and the invited address becomes the account's confirmed email. Pending invites are listed above the users and can be withdrawn.

When someone with a storage limit and a confirmed address passes 90% of it, Fileship emails them once, with a link to their storage page. Files in the trash count too. The warning is sent again only after their usage has dropped below 90%.

## Trash

Deleted files, folders and notes move to the trash for 30 days by default. Set **Trash retention** in **Administration → Instance** (or `NUXT_TRASH_RETENTION_DAYS`) to change it; `0` deletes everything immediately. Items in the trash still count toward a user's storage limit until they are deleted for good. Deleting a folder moves its subfolders to the trash with it, and anything deleted from a shared folder goes to the trash of the folder's owner. Files that members of a shared folder upload belong to the owner and count toward the owner's storage limit. Expired files and files that reach their view limit are always deleted immediately.

## Scheduled backups

Users can back up their account automatically in **Settings → Backups**: every day or every week, at an hour in their own time zone, keeping the newest automatic backups up to the number they choose. Backups they create or upload themselves are never deleted automatically. A user's backup limit counts both kinds, so a scheduled backup is skipped while manual backups alone fill it. Due backups are checked every 15 minutes, and an instance that was offline makes up a missed backup once when it starts again.

Turn **Scheduled backups** off in **Administration → Instance** (or set `NUXT_SCHEDULED_BACKUPS_ENABLED=false`) to stop them for everyone. Users' schedules are kept and continue when the setting is turned back on.

## Environment-only settings

A few settings can only be set through the environment, because they are needed before the dashboard starts or are fixed when the app is built:

| Variable                                      | Description                                                                     |
| --------------------------------------------- | ------------------------------------------------------------------------------- |
| `DATABASE_URL`                                | PostgreSQL connection string (required)                                         |
| `NUXT_DATA_DIRECTORY`                         | Local storage, thumbnails and temporary files (default: `data`)                 |
| `NUXT_UPDATER_URL` / `NUXT_UPDATER_SECRET`    | How the dashboard reaches the updater agent                                     |
| `NUXT_TELEMETRY_URL`                          | Where anonymous telemetry reports are sent                                      |
| `DELETE_EXPIRED_CRON` / `GENERATE_STATS_CRON` | Schedules for removing expired files and links, and for statistics (build time) |
| `ENABLE_PWA`                                  | Progressive Web App support (build time)                                        |

Build time options only take effect when you build the image yourself, for example with `docker-compose.dev.yml`. The published image always uses their defaults.

To set an environment variable with Docker Compose, add it to the shared `x-fileship` block in `docker-compose.yml`, so both the `fileship-blue` and `fileship-green` containers receive it:

```yaml
x-fileship: &fileship
    environment:
        NUXT_PUBLIC_REGISTRATION_ENABLED: 'true'
```

Then apply it with `docker compose up -d`.
