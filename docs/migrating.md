# Moving from another app

Fileship can take over a Zipline, XBackBone or Chibisafe instance: its accounts, its files with their original names, dates and link names, its folders or albums, its short links and its snippets. Super admins run the import from the dashboard, one app at a time.

| App       | Versions | What Fileship reads                             |
| --------- | -------- | ----------------------------------------------- |
| Zipline   | 3.7, 4.x | the server export (JSON) and the uploads folder |
| XBackBone | 3.x      | the SQLite database and the `storage` folder    |
| Chibisafe | 6.x      | the SQLite database and the `uploads` folder    |

## Before you start

- **Stop the old app**, or at least stop uploading to it. Files that arrive after you copied its data are not imported.
- **Keep a copy of its files** until you have checked the result. The import moves files instead of copying them a second time.
- **Make sure Fileship has room** for the files, on the disk or on the storage they will live on.

## Running the import

1. **Put the old app's files where Fileship can read them.** Copy its upload folder into Fileship's import folder, `data/import` next to `docker-compose.yml`. The folder can keep its name and layout: Fileship looks for each file by its path inside the old app's folder.

    ```sh
    cp -r /path/to/zipline/uploads ./data/import/zipline
    ```

    If the old app kept its files on S3, SFTP or FTP, add that bucket or folder in **Administration → Instance → Storage** instead (**Add storage**), with the same credentials the old app used.

2. In **Administration → Instance → Storage**, open the menu (**⋯**) of the storage that holds the files (**Data directory** for the import folder) and choose **Import from another app**.
3. **Choose the old app's data**, the export or database file described below. Fileship recognises the app and shows what it found, including how many files it can see on the storage. If some are missing, copy them and press **Check again**.
4. **Choose a Fileship account for each of the old app's accounts.** An account with the same username is picked if there is one, otherwise **New account**. You can pick any existing account instead, or **Don't import** to leave an account and everything it owns behind.
5. Press **Import** and confirm with your password. The progress shows on the Storage card, and the summary (imported, skipped, not found, failed) appears there and on the **Logs** page when it is done.

Fileship deletes its copy of the data file when the import finishes, or after six hours if you never start it. What is left in `data/import` afterwards (thumbnails, quarantined files, files of accounts you did not import) can be deleted.

## What comes over

|             | Zipline                                                                                                       | XBackBone                                                       | Chibisafe                                                         |
| ----------- | ------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------- | ----------------------------------------------------------------- |
| Accounts    | username, password, storage limit (by size)                                                                   | username, email address, storage limit (when quotas are on)     | username, storage limit                                           |
| Files       | original name, link name, upload date, views, view limit, expiry, folder; password protected files are locked | original name, link name, upload date; private files are locked | original name, link name, upload date, album                      |
| Folders     | nested folders                                                                                                | none (tags are not imported)                                    | albums become folders; a file in several albums goes to the first |
| Short links | code or vanity code, clicks, click limit; password protected links are locked, disabled links are skipped     | none                                                            | code, clicks                                                      |
| Notes       | none                                                                                                          | none                                                            | snippets become public notes, with code in a fenced block         |

Not imported: tags, favorites, avatars, two-factor authentication, passkeys, OAuth logins, invites, the old app's settings and statistics, and Chibisafe's NSFW flag. Thumbnails, AI analysis and search are made again by Fileship, like for any upload.

### Accounts

- New accounts keep the old username when it is free (Fileship usernames are 3 to 24 characters; a taken name gets a number). They can upload files, take notes and shorten links. **Admin rights are never copied**; grant them on the **Users** page.
- **Zipline accounts keep their password**, because Zipline and Fileship hash passwords the same way (Argon2); accounts that only signed in through OAuth have none and need one. XBackBone and Chibisafe use bcrypt, which Fileship cannot check, so set a new password for those accounts on the **Users** page. XBackBone email addresses are imported as confirmed addresses, so once [email](configuration.md#email) is set up those users can also reset their password themselves.
- New accounts get the storage limit they had in the old app, or no limit. An existing account keeps its own limit, and files that do not fit are skipped.
- Two-factor authentication and passkeys have to be set up again.

### Files and links

- **Files keep their link names** (`/u/<name>`) when the name is free and valid in Fileship (letters, digits, `-`, `_` and `.`); otherwise they get a new one. Zipline links (`/u/<name>`) keep working as soon as the old domain points at Fileship.
- **Locked files and links:** Zipline stores share passwords as hashes, and XBackBone has private files, while Fileship shows a share password to its owner. These items get a new random password, which the owner finds in the file's or link's details and can change.
- **Skipped:** expired files, files and links that reached their view or click limit, files the account already has (same name and size), files over the account's storage limit, items whose owner is not imported, files without an owner, and Chibisafe's quarantined files.
- **Not found:** files that are not on the storage, or whose size differs from the database (a copy that is still running).
- **Imported folders are public.** Zipline and Chibisafe serve every file by its link, whatever folder or album it is in, while Fileship hides the files of a private folder from visitors. So that old links keep working, folders come over public; their folder pages get new addresses that nobody has yet. Make a folder private afterwards to hide its files. A folder the account already has with the same name is used as it is.
- Running the import again is safe: accounts created the first time are picked again by their username, files that were imported are no longer in the import folder, and links and notes that already exist are skipped.

## Zipline

**Data:** the server export, made by a super admin.

- Zipline 4: **Server actions → Import / Export Data → Export Data**. Tick **Exclude Metrics Data** to keep the file small.
- Zipline 3.7: **Manage → Export all server data (JSON)**.

The export also contains Zipline's environment variables, including its secret and database password. Fileship skips that part and deletes its copy after the import; delete yours too.

**Files:** the uploads folder, `./uploads` next to Zipline's `docker-compose.yml` (`DATASOURCE_LOCAL_DIRECTORY`). With the S3 datasource, add the bucket as a storage; files under Zipline's subdirectory are found as well.

**Old links:** files keep `/u/<name>` (if you changed Zipline's `FILES_ROUTE`, redirect that route to `/u/` like below). Zipline's short links live under `/go/` (`URLS_ROUTE`) and Fileship's under `/s/`, so redirect them in your reverse proxy:

```caddyfile
@zipline path_regexp zipline ^/go/(.+)$
redir @zipline /s/{re.zipline.1} 301
```

```nginx
location ~ ^/go/(.+)$ {
    return 301 /s/$1;
}
```

## XBackBone

**Data:** the SQLite database, `resources/database/xbackbone.db` in XBackBone's folder (`/config/www/xbackbone/resources/database/xbackbone.db` in the linuxserver.io image). XBackBone installations on MySQL cannot be imported yet.

**Files:** the `storage` folder (`/config/www/xbackbone/storage` in the linuxserver.io image), with one folder per user. Copy the whole folder. With S3 or FTP storage, add it as a Fileship storage.

**Old links:** XBackBone links look like `/<user code>/<code>.<ext>`, and the file keeps `<code>.<ext>` as its link name, so redirect them:

```caddyfile
@xbackbone path_regexp xbackbone ^/[A-Za-z0-9]{5}/([A-Za-z0-9]+\.[A-Za-z0-9]+)$
redir @xbackbone /u/{re.xbackbone.1} 301
```

```nginx
location ~ ^/[A-Za-z0-9]{5}/([A-Za-z0-9]+\.[A-Za-z0-9]+)$ {
    return 301 /u/$1;
}
```

## Chibisafe

**Data:** the SQLite database, `database/database.sqlite` (the `./database` volume of Chibisafe's `docker-compose.yml`).

**Files:** the `uploads` folder (the `./uploads` volume). Its `thumbs`, `zips`, `tmp` and `quarantine` folders are not needed and stay behind. Files that Chibisafe stored on S3 are at the root of its bucket; add the bucket as a storage.

**Old links:** files are served at `/<name>` and short links at `/l/<code>`. Redirect them to `/u/<name>` and `/s/<code>`:

```caddyfile
@chibisafe-link path_regexp chibisafeLink ^/l/(.+)$
redir @chibisafe-link /s/{re.chibisafeLink.1} 301
@chibisafe-file path_regexp chibisafeFile ^/([A-Za-z0-9]{12}\.[A-Za-z0-9]+)$
redir @chibisafe-file /u/{re.chibisafeFile.1} 301
```

```nginx
location ~ ^/l/(.+)$ {
    return 301 /s/$1;
}

location ~ ^/([A-Za-z0-9]{12}\.[A-Za-z0-9]+)$ {
    return 301 /u/$1;
}
```

Chibisafe's generated file names are 12 characters long unless you changed **Generated filename length**; adjust the `12` to match. Matching that length keeps the redirect away from Fileship's own files, such as `/favicon.ico`.
