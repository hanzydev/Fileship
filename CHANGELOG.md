# Changelog

## v6.3.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v6.3.0...v6.3.1)

Fileship 6.3.1 makes single sign-on work with Google, keeps the storage page usable on large accounts and builds the Docker images without emulation, which also publishes the telemetry panel image that 6.3.0 missed.

### 🩹 Fixes

- **auth:** Send client credentials Google can read in single sign-on. Logging in with Google failed after consent with "The OAuth client was not found" ([b8be2edd](https://github.com/hanzydev/Fileship/commit/b8be2edd))
- **ui:** Keep the storage page lists in their own scroll area, so the duplicates are within reach on accounts with many files ([3de2ba15](https://github.com/hanzydev/Fileship/commit/3de2ba15))
- **ui:** Move the webhooks section to the left column of the instance page ([da01f519](https://github.com/hanzydev/Fileship/commit/da01f519))
- **docker:** Cross-build the images instead of building under QEMU ([52b5e396](https://github.com/hanzydev/Fileship/commit/52b5e396))

### 📖 Documentation

- Translate the README into the eleven other app languages ([9eaba2d7](https://github.com/hanzydev/Fileship/commit/9eaba2d7))
- Record the arm64 checks under emulation ([da455e62](https://github.com/hanzydev/Fileship/commit/da455e62))
- **extension:** Note that 1.1.0 is in review with the new listing texts ([ce1b9724](https://github.com/hanzydev/Fileship/commit/ce1b9724))

### 🤖 CI

- **extension:** Submit tagged releases to the Chrome Web Store ([38f77acd](https://github.com/hanzydev/Fileship/commit/38f77acd))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v6.3.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v6.2.0...v6.3.0)

Fileship 6.3 adds single sign-on with OpenID Connect, moves Zipline, XBackBone and Chibisafe instances into Fileship, speaks Spanish, Brazilian Portuguese and Russian, and ships Docker images that are six times smaller to download. It comes with Fileship for Windows 2.1, which signs in through single sign-on, and the Fileship Harbor fixes of 2.0.1 and 2.0.2.

### ✨ Highlights

- **Single sign-on.** Log in with any OpenID Connect provider: Fileship creates an account on the first login, matches an existing one by its confirmed email address, or lets you link your account from the security settings. Password login can be turned off for everyone except super admins ([guide](https://github.com/hanzydev/Fileship/blob/main/docs/configuration.md#single-sign-on)).
- **Move from Zipline, XBackBone and Chibisafe.** Super admins import another app's accounts, files with their link names and dates, folders, short links and snippets from a storage's menu. Zipline links keep working once the domain points at Fileship ([guide](https://github.com/hanzydev/Fileship/blob/main/docs/migrating.md)).
- **Three more languages.** Spanish, Brazilian Portuguese and Russian in the dashboard, emails, webhooks, the Windows app and the browser extension, for 12 languages in all. Corrections from native speakers are welcome.
- **Smaller Docker images.** The Fileship image is 230 MB to download instead of 1.43 GB (918 MB on disk instead of 5.74 GB), the updater agent 446 MB instead of 4.95 GB and the telemetry panel 500 MB instead of 2.77 GB.
- **Links on phones.** Link cards on small screens have the Open, Edit and Delete menu.
- **Fileship for Windows 2.1** ([fileship-desktop](https://github.com/hanzydev/fileship-desktop)):
    - Sign in with single sign-on, link your account and confirm sensitive actions with your provider in the Dashboard window.
    - Spanish, Brazilian Portuguese and Russian.
    - Fileship Harbor no longer crashes File Explorer in its folder or duplicates folders it cannot read, sends every delete to the Fileship trash (a deleted folder's files come back when you restore it), keeps edits saved during an upload and files that never reached Fileship, opens notes in full and applies remote changes without reloading everything.
    - Screenshots whose save to file failed are kept, and new installations go to `Programs\Fileship`.

### 🚀 Enhancements

- **i18n:** Add Spanish, Brazilian Portuguese and Russian ([34743888](https://github.com/hanzydev/Fileship/commit/34743888))
- **desktop:** Link and confirm single sign-on in the Dashboard window ([e9d06079](https://github.com/hanzydev/Fileship/commit/e9d06079))
- **ui:** Add Import from another app to the storage menu ([a9a009eb](https://github.com/hanzydev/Fileship/commit/a9a009eb))
- **api:** Import Zipline, XBackBone and Chibisafe instances ([b7751fa5](https://github.com/hanzydev/Fileship/commit/b7751fa5))
- **auth:** Add single sign-on with OpenID Connect ([e5031e8d](https://github.com/hanzydev/Fileship/commit/e5031e8d))

### 🔥 Performance

- **desktop:** Decrypt the session token once ([b7a460a2](https://github.com/hanzydev/Fileship/commit/b7a460a2))

### 🩹 Fixes

- **ui:** Keep longer labels inside their space ([9953e496](https://github.com/hanzydev/Fileship/commit/9953e496))
- **ui:** Show the link actions menu on mobile ([b2c99106](https://github.com/hanzydev/Fileship/commit/b2c99106))
- **api:** Repair Zipline 3 file names that were stored as Latin-1 ([4a379195](https://github.com/hanzydev/Fileship/commit/4a379195))
- **api:** Import other apps' folders as public so their file links keep working ([8162d0b8](https://github.com/hanzydev/Fileship/commit/8162d0b8))
- **desktop:** Refresh stale Harbor note placeholders when Harbor reloads ([74327549](https://github.com/hanzydev/Fileship/commit/74327549))
- **desktop:** Keep Harbor folder names within Fileship's limit ([602e919e](https://github.com/hanzydev/Fileship/commit/602e919e))
- **desktop:** Trash folders deleted together in Harbor as one tree ([e0e8ac8b](https://github.com/hanzydev/Fileship/commit/e0e8ac8b))
- **desktop:** Keep unsynced files when Harbor removes a deleted folder ([40131df2](https://github.com/hanzydev/Fileship/commit/40131df2))
- **desktop:** Upload edited Harbor files to the folder they are in ([15b624d6](https://github.com/hanzydev/Fileship/commit/15b624d6))
- **desktop:** Upload files put into a folder Harbor is still creating ([6f331480](https://github.com/hanzydev/Fileship/commit/6f331480))
- **desktop:** Stop Harbor from creating items for vanished paths ([56394846](https://github.com/hanzydev/Fileship/commit/56394846))
- **desktop:** Keep Harbor edits made during an upload ([ddc990a2](https://github.com/hanzydev/Fileship/commit/ddc990a2))
- **desktop:** Keep Harbor registered when its folder cannot be scanned ([9ddd8e69](https://github.com/hanzydev/Fileship/commit/9ddd8e69))
- **desktop:** Refresh stale Harbor notes instead of hydrating them truncated ([1df5a967](https://github.com/hanzydev/Fileship/commit/1df5a967))
- **desktop:** Hydrate pinned Harbor files off the watcher thread ([aaacc4f4](https://github.com/hanzydev/Fileship/commit/aaacc4f4))
- **desktop:** Reconnect the Harbor socket after the server refuses it ([f1457022](https://github.com/hanzydev/Fileship/commit/f1457022))
- **desktop:** Apply remote folder deletes to Harbor without a reload ([595efa99](https://github.com/hanzydev/Fileship/commit/595efa99))
- **desktop:** Coalesce Harbor resyncs ([c285963d](https://github.com/hanzydev/Fileship/commit/c285963d))
- **desktop:** Merge partial file and folder updates in Harbor ([0150bf97](https://github.com/hanzydev/Fileship/commit/0150bf97))
- **desktop:** Keep Harbor short links intact on click updates ([150092c9](https://github.com/hanzydev/Fileship/commit/150092c9))
- **desktop:** Use the light theme when Windows has no app theme set ([6aa2d8bc](https://github.com/hanzydev/Fileship/commit/6aa2d8bc))
- **desktop:** Run one update check at a time ([2e85a451](https://github.com/hanzydev/Fileship/commit/2e85a451))
- **desktop:** Keep screenshots whose save to file failed ([e16be703](https://github.com/hanzydev/Fileship/commit/e16be703))
- **desktop:** Fail uploads when a chunk cannot be read in full ([2a8f9847](https://github.com/hanzydev/Fileship/commit/2a8f9847))
- **desktop:** Send every Harbor delete to the Fileship trash ([9ee22f07](https://github.com/hanzydev/Fileship/commit/9ee22f07))
- **desktop:** Stop Harbor from crashing File Explorer ([bc765906](https://github.com/hanzydev/Fileship/commit/bc765906))
- **desktop:** Install new copies to Programs\Fileship ([64e35513](https://github.com/hanzydev/Fileship/commit/64e35513))
- **desktop:** Stop Harbor from duplicating folders it cannot read ([892e4ee1](https://github.com/hanzydev/Fileship/commit/892e4ee1))

### 💅 Refactors

- **i18n:** Pick plural forms with one shared rule ([8cfd38c2](https://github.com/hanzydev/Fileship/commit/8cfd38c2))
- **desktop:** Drop Harbor guards for payloads the server never sends ([aa8b065d](https://github.com/hanzydev/Fileship/commit/aa8b065d))
- **desktop:** Use the host's naming and theme helpers in the recorder ([2628340f](https://github.com/hanzydev/Fileship/commit/2628340f))

### 📖 Documentation

- Note that the slimmed images were only checked on amd64 ([1450e3fe](https://github.com/hanzydev/Fileship/commit/1450e3fe))
- Note how to test browser links and Keycloak re-authentication in the desktop app ([f3f4aebd](https://github.com/hanzydev/Fileship/commit/f3f4aebd))
- Add a guide for moving from Zipline, XBackBone and Chibisafe ([f9016bd6](https://github.com/hanzydev/Fileship/commit/f9016bd6))
- Document single sign-on ([eeb40fab](https://github.com/hanzydev/Fileship/commit/eeb40fab))
- Add a demo GIF, a comparison with other file hosts and a social preview ([8a6d3446](https://github.com/hanzydev/Fileship/commit/8a6d3446))
- Link the browser extension on the Chrome Web Store ([54eca6e7](https://github.com/hanzydev/Fileship/commit/54eca6e7))
- Note the Cloud Files nested delete limit and the Harbor payload rules ([91a71cfd](https://github.com/hanzydev/Fileship/commit/91a71cfd))
- Describe the Harbor and host fixes for Fileship for Windows 2.0.2 ([ad8587cd](https://github.com/hanzydev/Fileship/commit/ad8587cd))

### 📦 Build

- Deploy only the updater agent's production dependencies ([1ce50844](https://github.com/hanzydev/Fileship/commit/1ce50844))
- Ship only the server output and a traced Prisma CLI in the images ([8fb12f56](https://github.com/hanzydev/Fileship/commit/8fb12f56))
- Keep local build outputs and agent files out of the Docker context ([14b7d9b2](https://github.com/hanzydev/Fileship/commit/14b7d9b2))

### 🏡 Chore

- **desktop:** Bump Fileship for Windows to 2.0.2 ([3f20810a](https://github.com/hanzydev/Fileship/commit/3f20810a))
- **desktop:** Bump Fileship for Windows to 2.0.1 ([01c546f8](https://github.com/hanzydev/Fileship/commit/01c546f8))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v6.2.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v6.1.0...v6.2.0)

Fileship 6.2 lets you share folders and nest them, sends email, previews PDFs, text and code, finds duplicate files, backs up on a schedule and posts to webhooks. It comes with Fileship for Windows 2.0, rebuilt as a native app that uses a few megabytes of memory in the tray.

### ✨ Highlights

- **Shared folders.** Share a folder with other users as viewers or editors; everything inside stays in the owner's storage and trash.
- **Folders inside folders.** Folders nest up to ten levels, with breadcrumbs, moves between levels and public subfolders.
- **Email.** Add SMTP settings on the instance page, confirm your email address, reset a forgotten password, invite people by email and get a warning when your storage is almost full.
- **Previews.** PDFs open in the file viewer and get thumbnails, and text, code and Markdown files are shown with syntax highlighting.
- **Duplicate files.** The storage page lists files you uploaded more than once, and uploads ask before adding a copy.
- **Scheduled backups.** Back up daily or weekly at a time you choose, and keep as many automatic backups as your backup limit allows.
- **Webhooks.** Send uploads, deletes, short links and notes to Discord, Slack or any URL as signed JSON ([guide](https://github.com/hanzydev/Fileship/blob/main/docs/webhooks.md)).
- **Storage usage.** The sidebar, the users page and the stats show how much space you use and your limit.
- **Share to Fileship.** Installed as an app on Android, Fileship shows up in the share sheet for files, links and text.
- **Fileship for Windows 2.0** ([fileship-desktop](https://github.com/hanzydev/fileship-desktop)):
    - Rebuilt around a native app: it stays in the tray at a few megabytes of memory instead of 70-80 MB, and the app window only runs while it is open.
    - The region picker, annotations and the recording toolbar are native too, open faster and use far less memory while you capture, with the same look.
    - Recording records the microphone and system audio directly, using Windows 11's voice processing for the microphone.
    - Shapes keep their own colour and size: select a shape, even right after picking a region, to change its colour, size, background or shadow; shadows are easier to see.
    - The tray menu follows the dark theme.
    - Fileship Harbor syncs nested folders, keeps folders shared with you out of Explorer, and its status flyout gets a Settings button.
    - Needs 64-bit Windows 11 version 24H2 or newer. Your settings, history, Harbor folder and login are kept when you update.

### 🚀 Enhancements

- **desktop:** Require 64-bit Windows 11 24H2 or newer ([6fcfeee1](https://github.com/hanzydev/Fileship/commit/6fcfeee1))
- **desktop:** Run Electron only for windows, backed by the native host ([29283567](https://github.com/hanzydev/Fileship/commit/29283567))
- **desktop:** Run screen recordings from the native host ([7056e5f5](https://github.com/hanzydev/Fileship/commit/7056e5f5))
- **desktop:** Capture recording audio with WASAPI in the native host ([b47ffc5b](https://github.com/hanzydev/Fileship/commit/b47ffc5b))
- **desktop:** Add the ffmpeg manager and command builder to the native host ([a74b0bcd](https://github.com/hanzydev/Fileship/commit/a74b0bcd))
- **desktop:** Run Fileship Harbor in the native host ([d49a0b23](https://github.com/hanzydev/Fileship/commit/d49a0b23))
- **desktop:** Add a socket.io client over WinHTTP WebSocket ([6b88e76b](https://github.com/hanzydev/Fileship/commit/6b88e76b))
- **desktop:** Let the host take its pipe name from --pipe ([fd6c2e8a](https://github.com/hanzydev/Fileship/commit/fd6c2e8a))
- **desktop:** Add the native host core ([7287647b](https://github.com/hanzydev/Fileship/commit/7287647b))
- **desktop:** Add the native capture overlay and recorder toolbar ([6af4ea62](https://github.com/hanzydev/Fileship/commit/6af4ea62))
- **desktop:** Add the native host skeleton and shared C++ code ([24b1bc90](https://github.com/hanzydev/Fileship/commit/24b1bc90))
- **ui:** Preview text, code and markdown files in the file viewer ([5cc5b926](https://github.com/hanzydev/Fileship/commit/5cc5b926))
- Add webhooks for Discord, Slack and signed JSON ([aa72f799](https://github.com/hanzydev/Fileship/commit/aa72f799))
- **extension:** Add the Fileship browser extension ([3113b66c](https://github.com/hanzydev/Fileship/commit/3113b66c))
- **pwa:** Receive files, links and text from the share sheet ([e016ca9d](https://github.com/hanzydev/Fileship/commit/e016ca9d))
- Share folders with other users ([bc7d34fd](https://github.com/hanzydev/Fileship/commit/bc7d34fd))
- Preview PDFs ([0179f66e](https://github.com/hanzydev/Fileship/commit/0179f66e))
- Warn users by email when their storage is almost full ([80675ff3](https://github.com/hanzydev/Fileship/commit/80675ff3))
- Invite people by email ([15042385](https://github.com/hanzydev/Fileship/commit/15042385))
- Schedule automatic backups ([8ad794d4](https://github.com/hanzydev/Fileship/commit/8ad794d4))
- **auth:** Reset forgotten passwords by email ([0624a539](https://github.com/hanzydev/Fileship/commit/0624a539))
- Detect duplicate files ([81c67284](https://github.com/hanzydev/Fileship/commit/81c67284))
- **desktop:** Sync nested folders in Fileship Harbor ([a716585c](https://github.com/hanzydev/Fileship/commit/a716585c))
- Let users add and confirm an email address ([bdfb29a4](https://github.com/hanzydev/Fileship/commit/bdfb29a4))
- Nest folders inside folders ([eefc57b7](https://github.com/hanzydev/Fileship/commit/eefc57b7))
- Add SMTP settings and send a test email ([d40ce652](https://github.com/hanzydev/Fileship/commit/d40ce652))
- **desktop:** Extend the Harbor flyout and bring notes and links to Harbor ([33141d4e](https://github.com/hanzydev/Fileship/commit/33141d4e))
- Show storage usage and quotas in the dashboard ([8644b07d](https://github.com/hanzydev/Fileship/commit/8644b07d))
- **api:** Send view counts when a file is viewed ([354d8064](https://github.com/hanzydev/Fileship/commit/354d8064))

### 🩹 Fixes

- **desktop:** Draw the tray menu in the app theme ([8d5ae8f5](https://github.com/hanzydev/Fileship/commit/8d5ae8f5))
- **desktop:** Make the annotation shadow visible ([0ad8bf6c](https://github.com/hanzydev/Fileship/commit/0ad8bf6c))
- **desktop:** Let selected and edited shapes change their style ([c0a02075](https://github.com/hanzydev/Fileship/commit/c0a02075))
- **desktop:** Let Electron read app:quit before the host closes its pipe ([9c69e025](https://github.com/hanzydev/Fileship/commit/9c69e025))
- **desktop:** Decide whether the host is packaged from its install folder ([a0527b43](https://github.com/hanzydev/Fileship/commit/a0527b43))
- **desktop:** Connect the recorder to the overlay and the host settings ([44e16793](https://github.com/hanzydev/Fileship/commit/44e16793))
- **desktop:** Name the microphone field name in recording:microphones ([081b1dce](https://github.com/hanzydev/Fileship/commit/081b1dce))
- **desktop:** Remove nested Harbor folders before their parents ([2575c6fe](https://github.com/hanzydev/Fileship/commit/2575c6fe))
- **ui:** Show TypeScript .ts files as code on file cards and public folders ([18129488](https://github.com/hanzydev/Fileship/commit/18129488))
- **api:** Look up the MIME type by name when an upload has none ([55b334cd](https://github.com/hanzydev/Fileship/commit/55b334cd))
- Trim trailing slashes without a regular expression ([240bca3c](https://github.com/hanzydev/Fileship/commit/240bca3c))
- **telemetry:** Hash the admin password with scrypt ([cc88dd5b](https://github.com/hanzydev/Fileship/commit/cc88dd5b))
- **ui:** Redraw PDF pages when the viewer switches files ([c315a5e7](https://github.com/hanzydev/Fileship/commit/c315a5e7))
- **ui:** Hide upload controls in folders shared as viewer and show the role ([7507ef99](https://github.com/hanzydev/Fileship/commit/7507ef99))
- **ui:** Show who shared a folder only inside the folder ([b10f6a43](https://github.com/hanzydev/Fileship/commit/b10f6a43))
- **ui:** Show full upload errors and center the collapsed queue header ([75f4ab2f](https://github.com/hanzydev/Fileship/commit/75f4ab2f))
- **extension:** Keep picked files in IndexedDB until their upload ends ([b932b32e](https://github.com/hanzydev/Fileship/commit/b932b32e))
- **desktop:** Keep folders shared with you out of Harbor ([24adc575](https://github.com/hanzydev/Fileship/commit/24adc575))
- **desktop:** Log out of the app when the dashboard window logs out ([71cf8289](https://github.com/hanzydev/Fileship/commit/71cf8289))
- **desktop:** Show the Settings button in the Harbor status flyout ([aa1eb786](https://github.com/hanzydev/Fileship/commit/aa1eb786))

### 💅 Refactors

- **desktop:** Drop the 1.0.1 drive to harbor store migration ([83bab3e2](https://github.com/hanzydev/Fileship/commit/83bab3e2))
- **desktop:** Remove what the Vue overlay and recorder left behind ([78197d2c](https://github.com/hanzydev/Fileship/commit/78197d2c))
- **desktop:** Remove the N-API addon now that the host owns capture ([aa67a8f1](https://github.com/hanzydev/Fileship/commit/aa67a8f1))
- **desktop:** Split the native core into plain C++ functions ([adc94a08](https://github.com/hanzydev/Fileship/commit/adc94a08))
- **ui:** Move the upload options into UploadOptions ([5a8fcc47](https://github.com/hanzydev/Fileship/commit/5a8fcc47))

### 📖 Documentation

- **desktop:** Describe the native host, overlay and recording ([b86fc82c](https://github.com/hanzydev/Fileship/commit/b86fc82c))
- **extension:** Add Markdown descriptions for AMO ([2e1cb0d3](https://github.com/hanzydev/Fileship/commit/2e1cb0d3))
- **extension:** Explain the innerHTML warning to AMO reviewers ([61c2d470](https://github.com/hanzydev/Fileship/commit/61c2d470))
- **extension:** Fit the Chrome test instructions in 500 characters ([2aeaf7db](https://github.com/hanzydev/Fileship/commit/2aeaf7db))
- **extension:** Show the popups at their real size in the store images ([aaaa1dc8](https://github.com/hanzydev/Fileship/commit/aaaa1dc8))
- **extension:** Add store images ([4ad749c5](https://github.com/hanzydev/Fileship/commit/4ad749c5))
- **extension:** Add the privacy policy and the store listing ([19275535](https://github.com/hanzydev/Fileship/commit/19275535))
- Add the webhooks guide ([ee9fe318](https://github.com/hanzydev/Fileship/commit/ee9fe318))
- **extension:** Record the live test against a local dashboard ([1ed89642](https://github.com/hanzydev/Fileship/commit/1ed89642))
- Note dev traps found while testing folders ([61649b95](https://github.com/hanzydev/Fileship/commit/61649b95))
- Note how emails are translated and how to test them locally ([a2d0cd55](https://github.com/hanzydev/Fileship/commit/a2d0cd55))
- Note that the telemetry panel needs its Prisma client for typecheck ([9e725c27](https://github.com/hanzydev/Fileship/commit/9e725c27))
- Note that Harbor flyout commands need icons ([5e04da50](https://github.com/hanzydev/Fileship/commit/5e04da50))

### 🏡 Chore

- **desktop:** Bump Fileship for Windows to 2.0.0 ([6b4ce43d](https://github.com/hanzydev/Fileship/commit/6b4ce43d))
- **desktop:** Update dependencies and build steps for the native host ([32d2836d](https://github.com/hanzydev/Fileship/commit/32d2836d))
- **desktop:** Package the native host ([3cad99c8](https://github.com/hanzydev/Fileship/commit/3cad99c8))
- **desktop:** Add an overlay demo with a fixture renderer ([18c31573](https://github.com/hanzydev/Fileship/commit/18c31573))
- **desktop:** Copy the Geist font subsets for the native UI ([2c3af4ab](https://github.com/hanzydev/Fileship/commit/2c3af4ab))
- **deps:** Update nuxt, zod, Electron 45 and patched dependencies ([a5ed9afc](https://github.com/hanzydev/Fileship/commit/a5ed9afc))
- **deps:** Bump fast-uri from 3.1.8 to 3.1.9 ([7c824bbf](https://github.com/hanzydev/Fileship/commit/7c824bbf))
- **extension:** Make the store packages rebuild exactly ([6af5f0fa](https://github.com/hanzydev/Fileship/commit/6af5f0fa))
- **desktop:** Release 1.1.1 ([df877d90](https://github.com/hanzydev/Fileship/commit/df877d90))

#### ⚠️ Breaking Changes

- **desktop:** Require 64-bit Windows 11 24H2 or newer ([6fcfeee1](https://github.com/hanzydev/Fileship/commit/6fcfeee1))
- **desktop:** Drop the 1.0.1 drive to harbor store migration ([83bab3e2](https://github.com/hanzydev/Fileship/commit/83bab3e2))
- **desktop:** Run Electron only for windows, backed by the native host ([29283567](https://github.com/hanzydev/Fileship/commit/29283567))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v6.1.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v6.0.0...v6.1.0)

Fileship 6.1 speaks seven more languages, lets you skip the trash, and comes with Fileship for Windows 1.1, whose File Explorer integration is now called Fileship Harbor.

### ✨ Highlights

- **Seven new languages.** The dashboard and the Windows app are now available in German, French, Italian, Azerbaijani, Japanese, Korean and Simplified Chinese, next to English and Turkish, including API errors.
- **Delete forever.** The delete confirmation has a **Delete forever** button next to **Move to trash**, so files, folders and notes can skip the trash.
- **Touch friendly.** File selection, card menus and the code copy button show without hovering on phones and tablets.
- **Same drop screen everywhere.** Dragging files over the dashboard shows the same overlay as the Windows app.
- **Fileship for Windows 1.1** ([fileship-desktop](https://github.com/hanzydev/fileship-desktop)):
    - The File Explorer integration is now **Fileship Harbor**, with an explanation on its settings page. Your Harbor folder and settings are kept when you update.
    - Harbor shows thumbnails of images and videos without downloading them, adds Password, Expires and Sensitive status columns, a Share entry, the Windows 11 status flyout, and **Copy Fileship link**, **Open in browser** and **Show in Fileship** in the context menu.
    - Fixed files that stayed on the sync icon instead of the green check after an operation finished, and folders that never turned green.
    - **Upload to Fileship** in the Explorer context menu of any file.
    - Recent files get a right-click menu, **View in Fileship** (opens the file in the dashboard) and **Show in Fileship Harbor**.
    - A double click on the tray icon opens only the app; a single click still starts a region capture.
    - The emoji picker in the capture toolbar opens without stuttering.

### 🚀 Enhancements

- **desktop:** Add a context menu and View in Fileship to recent files ([b9fb884b](https://github.com/hanzydev/Fileship/commit/b9fb884b))
- Add Azerbaijani, Japanese and Korean ([2bed4284](https://github.com/hanzydev/Fileship/commit/2bed4284))
- **desktop:** Show Fileship Harbor file states and link actions in Explorer ([a7bdb13a](https://github.com/hanzydev/Fileship/commit/a7bdb13a))
- **desktop:** Add custom states, a share handler and the status flyout to the Harbor helper ([95ef16f4](https://github.com/hanzydev/Fileship/commit/95ef16f4))
- **desktop:** Keep Fileship Harbor items in pinned folders on the device ([619e5204](https://github.com/hanzydev/Fileship/commit/619e5204))
- **desktop:** Report Fileship Harbor sync errors and the trash to Explorer ([3db5fd69](https://github.com/hanzydev/Fileship/commit/3db5fd69))
- **desktop:** Host Fileship Harbor shell handlers in the drive helper ([ab60fb7f](https://github.com/hanzydev/Fileship/commit/ab60fb7f))
- **desktop:** Show Explorer thumbnails for Fileship Harbor files ([19b905ca](https://github.com/hanzydev/Fileship/commit/19b905ca))
- Add Simplified Chinese, German, French and Italian ([ffa767a7](https://github.com/hanzydev/Fileship/commit/ffa767a7))
- **desktop:** Call the drive Fileship Harbor and explain it in the settings ([6aa35e53](https://github.com/hanzydev/Fileship/commit/6aa35e53))
- **desktop:** Find the Fileship Harbor path of an uploaded file ([6bdaf5b7](https://github.com/hanzydev/Fileship/commit/6bdaf5b7))
- Delete files, folders and notes forever from the confirmation ([b5b5308c](https://github.com/hanzydev/Fileship/commit/b5b5308c))
- **ui:** Add a secondary action to the confirmation dialog ([4b5f7121](https://github.com/hanzydev/Fileship/commit/4b5f7121))
- **desktop:** Show uploaded files in Fileship Harbor ([6712030e](https://github.com/hanzydev/Fileship/commit/6712030e))
- **desktop:** Upload files from the Explorer context menu ([06da9f4f](https://github.com/hanzydev/Fileship/commit/06da9f4f))

### 🩹 Fixes

- **desktop:** Use an anchor icon for Fileship Harbor ([b2cc989f](https://github.com/hanzydev/Fileship/commit/b2cc989f))
- **desktop:** Keep the Harbor helper alive while Explorer holds a thumbnail provider ([a2859f14](https://github.com/hanzydev/Fileship/commit/a2859f14))
- **ui:** Make move to trash the primary action next to delete forever ([7593c56c](https://github.com/hanzydev/Fileship/commit/7593c56c))
- **desktop:** Keep Fileship drive items and folders marked in sync ([b820bf3c](https://github.com/hanzydev/Fileship/commit/b820bf3c))
- **desktop:** Animate the drop overlay and show card menus on touch ([a0b2fc1b](https://github.com/hanzydev/Fileship/commit/a0b2fc1b))
- **ui:** Show the code copy button on touch devices ([9fc8d9f4](https://github.com/hanzydev/Fileship/commit/9fc8d9f4))
- **ui:** Show file selection on touch devices ([5a8f532c](https://github.com/hanzydev/Fileship/commit/5a8f532c))
- **ui:** Match the dashboard drop overlay to the desktop app ([8a0923e5](https://github.com/hanzydev/Fileship/commit/8a0923e5))
- **desktop:** Open only the main window on a tray double click ([cef15311](https://github.com/hanzydev/Fileship/commit/cef15311))

### 🔥 Performance

- **desktop:** Open the emoji picker without stutter ([042255bd](https://github.com/hanzydev/Fileship/commit/042255bd))

### 💅 Refactors

- **desktop:** Rename the drive to Harbor in code, files and settings ([de9fb14d](https://github.com/hanzydev/Fileship/commit/de9fb14d))
- **desktop:** Share the drive helper's HTTP client ([973984ea](https://github.com/hanzydev/Fileship/commit/973984ea))

### 📖 Documentation

- List the manual checks for the new Windows app features ([6944c8e3](https://github.com/hanzydev/Fileship/commit/6944c8e3))
- Call the Explorer integration Harbor everywhere ([94effce9](https://github.com/hanzydev/Fileship/commit/94effce9))
- Call the Windows drive Fileship Harbor ([3397c9e0](https://github.com/hanzydev/Fileship/commit/3397c9e0))
- Link telemetry only from the documentation list ([8e818f76](https://github.com/hanzydev/Fileship/commit/8e818f76))
- Keep the README short and move the details to docs/ ([1c89c986](https://github.com/hanzydev/Fileship/commit/1c89c986))
- Add Fileship for Windows to the README ([e867bed9](https://github.com/hanzydev/Fileship/commit/e867bed9))

### 🏡 Chore

- List Simplified Chinese after the other Asian languages ([7c357efa](https://github.com/hanzydev/Fileship/commit/7c357efa))
- **desktop:** Pass extra arguments from the dev script to Electron ([bd096a3d](https://github.com/hanzydev/Fileship/commit/bd096a3d))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v6.0.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.4.4...v6.0.0)

Fileship 6 is a rewrite of the dashboard on a new design system, with storage backends, a trash, short links, a rebuilt AI pipeline, Turkish translations and a Windows app.

### ✨ Highlights

- **New dashboard.** Rebuilt on shadcn-vue with a shared design system, infinite lists, a command palette, a split-view file viewer, public folder, note and inbox pages, and light and dark themes with accent colors.
- **Storages.** Keep uploads and backups on the data folder, S3, SFTP or FTP, move everything to another storage, and import files a storage already holds.
- **Trash.** Deleted files, folders and notes stay in the trash for 30 days by default (`NUXT_TRASH_RETENTION_DAYS`).
- **Short links**, with passwords, click limits and expiry.
- **AI.** Low, medium and high presets with measured multilingual models: smart search that understands non-English queries, PaddleOCR text recognition, captions and model based sensitive content detection. Every model runs on a worker thread, so analysis never blocks requests.
- **Accounts.** Server-side sessions, passkeys, TOTP, optional self-registration, revocable upload tokens and impersonation.
- **Instance settings** in the dashboard: site name, uploads, trash, registration, Turnstile, AI, logs, telemetry and the active storage, without editing environment variables.
- **Translations.** The dashboard and the Windows app are available in English and Turkish, including API errors. The language follows the browser or the account setting.
- **Blue/green updates.** The updater starts the new version next to the old one behind Caddy and only switches when it is healthy.
- **Fileship for Windows** ([fileship-desktop](https://github.com/hanzydev/fileship-desktop)): ShareX-style capture across monitors with annotations, text capture (OCR), screen and GIF recording, uploads with history and the Fileship drive in File Explorer.

### ⚠️ Breaking changes

- **Sessions are rebuilt.** Every existing session is signed out. The session cookie is now the httpOnly `token`, and session tokens in ShareX configs no longer work: download the configs again from Settings → Integrations, which use upload tokens.
- **Environment variables use the `NUXT_` prefix**, so they can be changed at runtime: `DATA_DIRECTORY`, `REMOVE_EXIF_DATA`, `AVATAR_MAX_SIZE`, `LOG_LEVEL`, `TELEMETRY_ENABLED`, `UPDATER_URL`, `UPDATER_SECRET` and the `AI_*` variables become `NUXT_DATA_DIRECTORY`, `NUXT_REMOVE_EXIF_DATA` and so on. Most of these are now instance settings and only act as defaults.
- **New `docker-compose.yml`** with Caddy and two Fileship services (`fileship-blue`, `fileship-green`) for blue/green updates.
- **AI data is rebuilt.** New models are downloaded into `data/models`, every file is analyzed again in the background, and existing sensitive content flags are rewritten.
- **Backups** live on the storages and the backup upload endpoint takes `uploadId` and `chunkOffset` like file uploads.
- **Telemetry** sends one anonymous report (`POST /api/v2/report`) instead of the old events.

### ⬆️ Upgrading from 5.x

The upgrade was tested from a 5.4.4 installation with files, folders and notes: all database migrations apply on top of the 5.x database, and files, folders, notes and passwords are kept.

1. Back up the database and the `data` folder.
2. Replace your `docker-compose.yml` with the new one, keep your `data` folder next to it, and rename the variables in your `.env` as listed above (`SECRET_TOKEN` stays).
3. Run `docker compose pull` and `docker compose up -d`. Migrations run on startup.
4. Sign in again and download new ShareX configs if you use them.

5.x installations with the auto-updater turned on pull the new image into their old compose file. That works, but until the compose file is replaced the dashboard's update button cannot reach the updater (the old `UPDATER_*` variables are no longer read) and blue/green updates are not used.

### 🚀 Enhancements

- Add version info to sidebar ([60f37919](https://github.com/hanzydev/Fileship/commit/60f37919))
- Stat cards ([0290fee2](https://github.com/hanzydev/Fileship/commit/0290fee2))
- **ui:** Theme system, command palette & shared primitives ([d7f30240](https://github.com/hanzydev/Fileship/commit/d7f30240))
- **ui:** Global upload experience ([66d60dd1](https://github.com/hanzydev/Fileship/commit/66d60dd1))
- **ui:** Files manager ([2afbbbda](https://github.com/hanzydev/Fileship/commit/2afbbbda))
- **ui:** Home overview ([1db61d0b](https://github.com/hanzydev/Fileship/commit/1db61d0b))
- **ui:** Folders ([7f943d63](https://github.com/hanzydev/Fileship/commit/7f943d63))
- **ui:** Notes workspace ([7896157d](https://github.com/hanzydev/Fileship/commit/7896157d))
- **ui:** Settings ([4a3b5cac](https://github.com/hanzydev/Fileship/commit/4a3b5cac))
- **ui:** Admin pages ([e3ad471e](https://github.com/hanzydev/Fileship/commit/e3ad471e))
- **ui:** Public view pages ([9e663cc4](https://github.com/hanzydev/Fileship/commit/9e663cc4))
- **ui:** Updater card ([344085fb](https://github.com/hanzydev/Fileship/commit/344085fb))
- **api:** Paginated lists ([c4899f2a](https://github.com/hanzydev/Fileship/commit/c4899f2a))
- **ui:** Infinite lists ([7d185048](https://github.com/hanzydev/Fileship/commit/7d185048))
- **updater:** Blue/green deploys behind caddy ([449a566b](https://github.com/hanzydev/Fileship/commit/449a566b))
- **ui:** Home activity, settings and toolbars ([16487746](https://github.com/hanzydev/Fileship/commit/16487746))
- **ui:** Shadcn drop zones ([8da275ed](https://github.com/hanzydev/Fileship/commit/8da275ed))
- **auth:** Name passkeys after their provider ([c343e500](https://github.com/hanzydev/Fileship/commit/c343e500))
- **ui:** Reveal toggle on password inputs ([a79cb348](https://github.com/hanzydev/Fileship/commit/a79cb348))
- **auth:** Optional self-registration ([f433fd9e](https://github.com/hanzydev/Fileship/commit/f433fd9e))
- **api:** Instance settings store ([01ea7134](https://github.com/hanzydev/Fileship/commit/01ea7134))
- **ui:** Instance settings page ([99b7b4a2](https://github.com/hanzydev/Fileship/commit/99b7b4a2))
- **ui:** Split-view file viewer ([ba36ac92](https://github.com/hanzydev/Fileship/commit/ba36ac92))
- **ui:** File viewer thumbnail strip ([284e1374](https://github.com/hanzydev/Fileship/commit/284e1374))
- **api:** Generate image thumbnails ([74363e12](https://github.com/hanzydev/Fileship/commit/74363e12))
- Analyze media metadata ([69851b2a](https://github.com/hanzydev/Fileship/commit/69851b2a))
- **ui:** Remember the files view mode ([166186a2](https://github.com/hanzydev/Fileship/commit/166186a2))
- ⚠️ **auth:** Rebuild sessions ([bd5be957](https://github.com/hanzydev/Fileship/commit/bd5be957))
- Revocable upload tokens ([5196844a](https://github.com/hanzydev/Fileship/commit/5196844a))
- **ui:** Macos and linux integrations ([e14d0fa6](https://github.com/hanzydev/Fileship/commit/e14d0fa6))
- **ui:** Close the file viewer from empty space ([9c385b6c](https://github.com/hanzydev/Fileship/commit/9c385b6c))
- **ui:** File viewer loading skeleton ([a27bd9df](https://github.com/hanzydev/Fileship/commit/a27bd9df))
- **ui:** File card loading skeleton ([bce9f988](https://github.com/hanzydev/Fileship/commit/bce9f988))
- **ui:** Skeletons while loading more items ([f5076ee8](https://github.com/hanzydev/Fileship/commit/f5076ee8))
- **api:** Remove files missing from storage on startup ([720fde6b](https://github.com/hanzydev/Fileship/commit/720fde6b))
- **api:** Pluggable storage drivers ([fc2c064c](https://github.com/hanzydev/Fileship/commit/fc2c064c))
- **api:** Stream chunked uploads straight to storage ([8ea771a6](https://github.com/hanzydev/Fileship/commit/8ea771a6))
- ⚠️ **api:** Keep backups on the storage backends ([d16ce4e7](https://github.com/hanzydev/Fileship/commit/d16ce4e7))
- **ui:** Storage settings ([c8319d4b](https://github.com/hanzydev/Fileship/commit/c8319d4b))
- **api:** Trash for files, folders and notes ([1039975a](https://github.com/hanzydev/Fileship/commit/1039975a))
- **ui:** Trash ([6498e30d](https://github.com/hanzydev/Fileship/commit/6498e30d))
- **api:** Link shortener ([92c3b99d](https://github.com/hanzydev/Fileship/commit/92c3b99d))
- **ui:** Link shortener ([0bf60618](https://github.com/hanzydev/Fileship/commit/0bf60618))
- **ui:** File previews in the trash ([135f68d7](https://github.com/hanzydev/Fileship/commit/135f68d7))
- **ui:** Pick folder files from a preview grid ([f3043cd5](https://github.com/hanzydev/Fileship/commit/f3043cd5))
- **api:** Show locked files in public folders ([8baee202](https://github.com/hanzydev/Fileship/commit/8baee202))
- **ui:** Unlock files in public folders ([e9f68cc7](https://github.com/hanzydev/Fileship/commit/e9f68cc7))
- ⚠️ **api:** AI presets with measured models and a reliable analysis queue ([40be0d47](https://github.com/hanzydev/Fileship/commit/40be0d47))
- **ui:** Show analysis progress in file insights ([f9f5da66](https://github.com/hanzydev/Fileship/commit/f9f5da66))
- **api:** Reset AI data ([3f3adbc7](https://github.com/hanzydev/Fileship/commit/3f3adbc7))
- **ui:** Reset AI data from the instance settings ([c234f590](https://github.com/hanzydev/Fileship/commit/c234f590))
- **ui:** Find short links in the command palette ([7d61b104](https://github.com/hanzydev/Fileship/commit/7d61b104))
- ⚠️ **telemetry:** Send one anonymous report with usage and features ([ea7d576c](https://github.com/hanzydev/Fileship/commit/ea7d576c))
- **telemetry-panel:** Receive reports and keep daily statistics ([eef038a3](https://github.com/hanzydev/Fileship/commit/eef038a3))
- **telemetry-panel:** Overview, versions, system, features and instances pages ([0b013839](https://github.com/hanzydev/Fileship/commit/0b013839))
- **telemetry-panel:** Import the old panel's reports ([bf06d879](https://github.com/hanzydev/Fileship/commit/bf06d879))
- **ui:** Show a short links card on the home page of link-only users ([6a93e04e](https://github.com/hanzydev/Fileship/commit/6a93e04e))
- **telemetry-panel:** Serve assets from /_hanzy_was_here/ like the dashboard ([45c18ab5](https://github.com/hanzydev/Fileship/commit/45c18ab5))
- **api:** Accept session tokens as bearer tokens ([26f41825](https://github.com/hanzydev/Fileship/commit/26f41825))
- **api:** Show desktop app sessions as Fileship for Windows ([1c43ece4](https://github.com/hanzydev/Fileship/commit/1c43ece4))
- Store and show the original name of every file ([72e21597](https://github.com/hanzydev/Fileship/commit/72e21597))
- **desktop:** Fileship for Windows with login, dashboard window and updates ([cdff9f3c](https://github.com/hanzydev/Fileship/commit/cdff9f3c))
- **desktop:** Screen capture, chunked uploads, hotkeys and history ([a89f214b](https://github.com/hanzydev/Fileship/commit/a89f214b))
- **desktop:** Region capture overlay with window detection and magnifier ([b6d2f701](https://github.com/hanzydev/Fileship/commit/b6d2f701))
- **desktop:** Capture history and hotkey, capture and upload settings ([7766d4e2](https://github.com/hanzydev/Fileship/commit/7766d4e2))
- **desktop:** Screen and GIF recording with ffmpeg, microphone and system audio ([329cfa43](https://github.com/hanzydev/Fileship/commit/329cfa43))
- **desktop:** Annotate captures in the region overlay and in an editor window ([9b421911](https://github.com/hanzydev/Fileship/commit/9b421911))
- **desktop:** Native Cloud Files helper for the Fileship drive ([94ae7c6b](https://github.com/hanzydev/Fileship/commit/94ae7c6b))
- **desktop:** Sync the Fileship drive with File Explorer ([94d6fe85](https://github.com/hanzydev/Fileship/commit/94d6fe85))
- **desktop:** Drive settings, tray entry and packaging ([c6dfe271](https://github.com/hanzydev/Fileship/commit/c6dfe271))
- **api:** Accept upload chunks in parallel and in any order ([e85e9638](https://github.com/hanzydev/Fileship/commit/e85e9638))
- **desktop:** Parallel chunk uploads with progress ([54e3b03b](https://github.com/hanzydev/Fileship/commit/54e3b03b))
- **desktop:** Delete drive files permanently when Windows deletes them ([094c25f5](https://github.com/hanzydev/Fileship/commit/094c25f5))
- **desktop:** Show drive upload progress in File Explorer ([9d3cfc8f](https://github.com/hanzydev/Fileship/commit/9d3cfc8f))
- **desktop:** Native core addon ([6adb220b](https://github.com/hanzydev/Fileship/commit/6adb220b))
- **ui:** Mark sensitive files on file cards ([9e644ca0](https://github.com/hanzydev/Fileship/commit/9e644ca0))
- **desktop:** Play a shutter sound when a screenshot is taken ([35b316a9](https://github.com/hanzydev/Fileship/commit/35b316a9))
- **desktop:** Play a chime when an upload finishes ([a7e2264f](https://github.com/hanzydev/Fileship/commit/a7e2264f))
- **api:** Recognise text in an uploaded image ([b68ad91c](https://github.com/hanzydev/Fileship/commit/b68ad91c))
- **desktop:** Copy text from a region with OCR ([89fbb84f](https://github.com/hanzydev/Fileship/commit/89fbb84f))
- **desktop:** Capture the active monitor with Print Screen by default ([20c0e97c](https://github.com/hanzydev/Fileship/commit/20c0e97c))
- **desktop:** List hotkeys with defaults first ([3657978f](https://github.com/hanzydev/Fileship/commit/3657978f))
- **desktop:** Capture the region as soon as the mouse is released ([b3cbb379](https://github.com/hanzydev/Fileship/commit/b3cbb379))
- **desktop:** Annotate anywhere before picking the region ([06b51f45](https://github.com/hanzydev/Fileship/commit/06b51f45))
- **desktop:** Move the capture toolbar across monitors ([2aaa89c3](https://github.com/hanzydev/Fileship/commit/2aaa89c3))
- **api:** Import files a storage already holds ([1059387d](https://github.com/hanzydev/Fileship/commit/1059387d))
- **ui:** Add i18n to the shared layer ([e2a7369e](https://github.com/hanzydev/Fileship/commit/e2a7369e))
- **api:** Store the user's language and translate API errors ([943790f8](https://github.com/hanzydev/Fileship/commit/943790f8))
- **dashboard:** Translate the dashboard into Turkish ([83dc6d8f](https://github.com/hanzydev/Fileship/commit/83dc6d8f))
- **desktop:** Translate the app into Turkish ([5a61a440](https://github.com/hanzydev/Fileship/commit/5a61a440))
- **dashboard:** Pick the language from the account menu ([4bc5c552](https://github.com/hanzydev/Fileship/commit/4bc5c552))
- ⚠️ **desktop:** Remove the separate image editor ([23f3fb00](https://github.com/hanzydev/Fileship/commit/23f3fb00))

### 🩹 Fixes

- **ui:** Render theme toggle icon without hydration mismatch ([6cc3fc59](https://github.com/hanzydev/Fileship/commit/6cc3fc59))
- **ui:** Review feedback ([3935c9dd](https://github.com/hanzydev/Fileship/commit/3935c9dd))
- **stats:** Keep view history and compute growth over 24 hours ([b44779cc](https://github.com/hanzydev/Fileship/commit/b44779cc))
- **ui:** Shadcn upstream bugs ([09171231](https://github.com/hanzydev/Fileship/commit/09171231))
- **ui:** Navigation, selects and cursors ([eb26eadd](https://github.com/hanzydev/Fileship/commit/eb26eadd))
- **ui:** Tables ([7d3677bf](https://github.com/hanzydev/Fileship/commit/7d3677bf))
- **ui:** Cards and skeletons ([53f0702d](https://github.com/hanzydev/Fileship/commit/53f0702d))
- **updater:** Docker desktop paths and faster rollback ([cd5ee649](https://github.com/hanzydev/Fileship/commit/cd5ee649))
- **ui:** Home layout ([c0ca3dcc](https://github.com/hanzydev/Fileship/commit/c0ca3dcc))
- **ui:** Table widths, mobile skeletons and sorting ([d87671dd](https://github.com/hanzydev/Fileship/commit/d87671dd))
- **ui:** Command padding, folder create, select and arrows ([cf507c39](https://github.com/hanzydev/Fileship/commit/cf507c39))
- **api:** Security ([1bf2bb2e](https://github.com/hanzydev/Fileship/commit/1bf2bb2e))
- **api:** Correctness ([ec562474](https://github.com/hanzydev/Fileship/commit/ec562474))
- **updater:** Wait for restores ([81751af3](https://github.com/hanzydev/Fileship/commit/81751af3))
- Client logic ([f1c768be](https://github.com/hanzydev/Fileship/commit/f1c768be))
- **api:** Clear note links when notes turn private ([6ce2ada4](https://github.com/hanzydev/Fileship/commit/6ce2ada4))
- **ui:** Shell and navigation ([0e656dff](https://github.com/hanzydev/Fileship/commit/0e656dff))
- **ui:** Settings ([3da4edf3](https://github.com/hanzydev/Fileship/commit/3da4edf3))
- **ui:** Public pages and login ([1516977c](https://github.com/hanzydev/Fileship/commit/1516977c))
- **ui:** Lists, toolbars and sheets ([774321c2](https://github.com/hanzydev/Fileship/commit/774321c2))
- **ui:** File card overlay controls ([9143cb76](https://github.com/hanzydev/Fileship/commit/9143cb76))
- **ui:** Notes active ring in dark mode ([44492c6c](https://github.com/hanzydev/Fileship/commit/44492c6c))
- **ui:** Loading indicator color ([27b9fad4](https://github.com/hanzydev/Fileship/commit/27b9fad4))
- **ui:** Command and select insets ([3466c369](https://github.com/hanzydev/Fileship/commit/3466c369))
- **ui:** Inbox layout and file rows ([737980ac](https://github.com/hanzydev/Fileship/commit/737980ac))
- **auth:** Passkey login with unknown credentials ([f4341734](https://github.com/hanzydev/Fileship/commit/f4341734))
- **ui:** Login page ([7153e74d](https://github.com/hanzydev/Fileship/commit/7153e74d))
- Avoid buildPublicUrl auto-import clash between app and server ([86c885dc](https://github.com/hanzydev/Fileship/commit/86c885dc))
- **auth:** Report unverified passkey registrations ([6f5636ba](https://github.com/hanzydev/Fileship/commit/6f5636ba))
- **ui:** File sheet close animation ([0b3c09a0](https://github.com/hanzydev/Fileship/commit/0b3c09a0))
- **ui:** Blur private folder previews ([a5d60749](https://github.com/hanzydev/Fileship/commit/a5d60749))
- **ui:** Hide header behind password gate ([83e337c9](https://github.com/hanzydev/Fileship/commit/83e337c9))
- **ui:** Match notes column gap to page padding ([0f1c00ae](https://github.com/hanzydev/Fileship/commit/0f1c00ae))
- **ui:** Center public switch in folder dialog ([68333f10](https://github.com/hanzydev/Fileship/commit/68333f10))
- **ui:** Two-column instance settings ([ca7f1c7a](https://github.com/hanzydev/Fileship/commit/ca7f1c7a))
- **ui:** Balanced instance settings columns ([68ee64b6](https://github.com/hanzydev/Fileship/commit/68ee64b6))
- **ui:** Animate file card hover zoom ([0c224ecc](https://github.com/hanzydev/Fileship/commit/0c224ecc))
- **ui:** Zoom file viewer images at the clicked point ([8c793f5f](https://github.com/hanzydev/Fileship/commit/8c793f5f))
- **ui:** Load more files in the home page viewer ([76fb4517](https://github.com/hanzydev/Fileship/commit/76fb4517))
- **ui:** Show thumbnails in file previews ([e1b7779b](https://github.com/hanzydev/Fileship/commit/e1b7779b))
- **ui:** Consistent page spacing ([35ae458a](https://github.com/hanzydev/Fileship/commit/35ae458a))
- **ui:** Escape raw html in markdown ([ab5e3d1a](https://github.com/hanzydev/Fileship/commit/ab5e3d1a))
- **api:** Drop the session token index with its column ([2e84687d](https://github.com/hanzydev/Fileship/commit/2e84687d))
- **ui:** List media metadata with the file details ([9615a2db](https://github.com/hanzydev/Fileship/commit/9615a2db))
- **ui:** Read the upload url from formatted responses in scripts ([2a49b4d1](https://github.com/hanzydev/Fileship/commit/2a49b4d1))
- **ui:** Keep file viewer arrows in place when pressed ([d2ae2386](https://github.com/hanzydev/Fileship/commit/d2ae2386))
- **ui:** Animate confirmation dialogs closing ([a0d53172](https://github.com/hanzydev/Fileship/commit/a0d53172))
- **ui:** File bulk bar select all icon, delete color and mobile layout ([704ba507](https://github.com/hanzydev/Fileship/commit/704ba507))
- **ui:** Animate the drop zone overlay closing ([0c1d488d](https://github.com/hanzydev/Fileship/commit/0c1d488d))
- **api:** Bundle the ffprobe binary in the server build ([6c32b293](https://github.com/hanzydev/Fileship/commit/6c32b293))
- **api:** Detect cross-site writes with Sec-Fetch-Site ([ae92c710](https://github.com/hanzydev/Fileship/commit/ae92c710))
- **ui:** Name the sharex config fileship.sxcu ([9ad19b61](https://github.com/hanzydev/Fileship/commit/9ad19b61))
- **api:** Only send S3 checksums when required ([fdd29fd5](https://github.com/hanzydev/Fileship/commit/fdd29fd5))
- **ui:** Show a spinner while setting the active storage ([79e378f6](https://github.com/hanzydev/Fileship/commit/79e378f6))
- **ui:** Command palette radius and padding ([343d68c7](https://github.com/hanzydev/Fileship/commit/343d68c7))
- **ui:** Keep confirmation dialogs above the file viewer ([1e40f004](https://github.com/hanzydev/Fileship/commit/1e40f004))
- **ui:** Pick link expiry like file expiry ([f7262168](https://github.com/hanzydev/Fileship/commit/f7262168))
- **ui:** Format stat card numbers ([36fa8972](https://github.com/hanzydev/Fileship/commit/36fa8972))
- **ui:** Align trash row menu with other tables ([b3e1c173](https://github.com/hanzydev/Fileship/commit/b3e1c173))
- **ui:** Trash tab switching ([f786a9bd](https://github.com/hanzydev/Fileship/commit/f786a9bd))
- **ui:** Trash file viewer tabs and open link ([ea9601f8](https://github.com/hanzydev/Fileship/commit/ea9601f8))
- **ui:** Show progress on async buttons ([195382f6](https://github.com/hanzydev/Fileship/commit/195382f6))
- **ui:** Loading toasts for menu actions ([aef48429](https://github.com/hanzydev/Fileship/commit/aef48429))
- **api:** Leave sensitive files out of public folder archives ([b3de784d](https://github.com/hanzydev/Fileship/commit/b3de784d))
- ⚠️ **api:** Precise sensitive content detection ([36ba1e52](https://github.com/hanzydev/Fileship/commit/36ba1e52))
- ⚠️ **api:** Multilingual smart search ([7631dddf](https://github.com/hanzydev/Fileship/commit/7631dddf))
- ⚠️ **api:** Model based sensitive content detection ([dd96eb7f](https://github.com/hanzydev/Fileship/commit/dd96eb7f))
- **ui:** Show search progress while results refresh ([3e1e4dd5](https://github.com/hanzydev/Fileship/commit/3e1e4dd5))
- **api:** Hide unanalyzed files from visitors ([bad4a862](https://github.com/hanzydev/Fileship/commit/bad4a862))
- **ui:** Show details when a file has no insights ([3f3129c6](https://github.com/hanzydev/Fileship/commit/3f3129c6))
- **ui:** Center the authenticator app button ([f942e43f](https://github.com/hanzydev/Fileship/commit/f942e43f))
- **api:** Make the v6 AI migrations safe to run again ([95a3ef5c](https://github.com/hanzydev/Fileship/commit/95a3ef5c))
- **api:** Handle files without PII reasons in the cleanup migration ([5c96c64e](https://github.com/hanzydev/Fileship/commit/5c96c64e))
- **api:** Let self-registered users shorten links by default ([bf4db327](https://github.com/hanzydev/Fileship/commit/bf4db327))
- **api:** Give link shorteners an upload token ([553bf675](https://github.com/hanzydev/Fileship/commit/553bf675))
- **api:** Rate limit failed file and inbox passwords everywhere ([b4314355](https://github.com/hanzydev/Fileship/commit/b4314355))
- **ui:** Refresh short links when domains change ([8e5e2d17](https://github.com/hanzydev/Fileship/commit/8e5e2d17))
- **api:** Analyze files restored from the trash ([3cb1d41a](https://github.com/hanzydev/Fileship/commit/3cb1d41a))
- **api:** Leave trashed files out of per-user file counts ([77c222da](https://github.com/hanzydev/Fileship/commit/77c222da))
- **api:** Log account, storage and AI admin actions as important ([6a446301](https://github.com/hanzydev/Fileship/commit/6a446301))
- **ui:** Mention short links in user deletion and backups ([1fcc534f](https://github.com/hanzydev/Fileship/commit/1fcc534f))
- **telemetry:** Measure the data disk from relative paths and accept any UUID id ([bceaf86d](https://github.com/hanzydev/Fileship/commit/bceaf86d))
- **telemetry-panel:** Stretch the cards of an incomplete last row ([3392be00](https://github.com/hanzydev/Fileship/commit/3392be00))
- **api:** Validate backup restores like the create endpoints ([79cd8047](https://github.com/hanzydev/Fileship/commit/79cd8047))
- **api:** Ask for verification when an edit grants admin rights ([c5071cd5](https://github.com/hanzydev/Fileship/commit/c5071cd5))
- Inbox, AI and integration settings follow the user's permissions ([acf95db5](https://github.com/hanzydev/Fileship/commit/acf95db5))
- **api:** Rate limit public folder archives and link unlocks per link ([e93e2433](https://github.com/hanzydev/Fileship/commit/e93e2433))
- **api:** Log passkey and session changes and keep backup and trash logs at the important level ([6ba2d32d](https://github.com/hanzydev/Fileship/commit/6ba2d32d))
- Leave pages the user lost access to and refresh the trash after restores and expiry ([1f78d8ed](https://github.com/hanzydev/Fileship/commit/1f78d8ed))
- **ui:** Hide the Update button and super admin edits from regular admins ([318d924f](https://github.com/hanzydev/Fileship/commit/318d924f))
- **telemetry-panel:** Harden logins and report ingestion ([f047797f](https://github.com/hanzydev/Fileship/commit/f047797f))
- **telemetry-panel:** Labels and numbers that did not match the data ([a4618d54](https://github.com/hanzydev/Fileship/commit/a4618d54))
- **updater:** Refuse update requests when SECRET_TOKEN is not set ([e01243a3](https://github.com/hanzydev/Fileship/commit/e01243a3))
- **telemetry:** Validate the report against the schema before sending it ([15dea96d](https://github.com/hanzydev/Fileship/commit/15dea96d))
- **api:** Accept an absolute NUXT_DATA_DIRECTORY and drop the unused site description ([45cd3b5b](https://github.com/hanzydev/Fileship/commit/45cd3b5b))
- **desktop:** Bring the capture overlay to the foreground so keys work before a selection ([07eb5b6d](https://github.com/hanzydev/Fileship/commit/07eb5b6d))
- **desktop:** Movable capture toolbar at the top, uniform buttons and animated highlights ([eaaaec5c](https://github.com/hanzydev/Fileship/commit/eaaaec5c))
- **desktop:** Instant recording overlay, one ffmpeg prompt and a cleaner recorder window ([f87ce264](https://github.com/hanzydev/Fileship/commit/f87ce264))
- **desktop:** Magnetic capture toolbar edges with free placement ([a6514f3b](https://github.com/hanzydev/Fileship/commit/a6514f3b))
- **desktop:** Center the toolbar separators ([e6cfc402](https://github.com/hanzydev/Fileship/commit/e6cfc402))
- **desktop:** Keep the copied image in clipboard history before the link replaces it ([62577664](https://github.com/hanzydev/Fileship/commit/62577664))
- **desktop:** Fileship icon and version info for the drive helper and app ([5d6fc9ff](https://github.com/hanzydev/Fileship/commit/5d6fc9ff))
- **desktop:** Separate app identity for development builds ([35e9dcf2](https://github.com/hanzydev/Fileship/commit/35e9dcf2))
- **api:** List trashed files when there is no search ([16d4ef2a](https://github.com/hanzydev/Fileship/commit/16d4ef2a))
- **desktop:** Theme background for windows before they paint ([a39a8ddf](https://github.com/hanzydev/Fileship/commit/a39a8ddf))
- **ui:** Paint the theme's canvas before the stylesheets load ([05f9f05f](https://github.com/hanzydev/Fileship/commit/05f9f05f))
- **ui:** Match sonner toasts to the design system ([34fc8c63](https://github.com/hanzydev/Fileship/commit/34fc8c63))
- **desktop:** Never adopt or delete files the drive does not own ([badbced0](https://github.com/hanzydev/Fileship/commit/badbced0))
- **api:** Resolve client IPs from the trusted proxy only ([a83edb77](https://github.com/hanzydev/Fileship/commit/a83edb77))
- **api:** Serve the last allowed view and hide files visitors may not see ([e0433180](https://github.com/hanzydev/Fileship/commit/e0433180))
- **api:** Bind upload sessions to their endpoint and check every chunk ([e95eb77d](https://github.com/hanzydev/Fileship/commit/e95eb77d))
- Block unsafe markdown URLs and count text views only in the browser ([e01999ec](https://github.com/hanzydev/Fileship/commit/e01999ec))
- **api:** Require verification to test a storage with its stored secrets ([8d18897c](https://github.com/hanzydev/Fileship/commit/8d18897c))
- **api:** Keep storage sync away from uploads that are still finishing ([e780b499](https://github.com/hanzydev/Fileship/commit/e780b499))
- **ui:** Show the file icon only on cards without media ([7effa4fe](https://github.com/hanzydev/Fileship/commit/7effa4fe))
- **api:** Always finish backup restores and validate restored file names ([be254fff](https://github.com/hanzydev/Fileship/commit/be254fff))
- **ui:** Keep note edits made while a save is in flight ([a9e962f1](https://github.com/hanzydev/Fileship/commit/a9e962f1))
- **api:** Merge user limits with the stored ones and require whole numbers ([9578204d](https://github.com/hanzydev/Fileship/commit/9578204d))
- **desktop:** Retry the drive start with backoff and serialize start and stop ([55602590](https://github.com/hanzydev/Fileship/commit/55602590))
- **api:** Require verification to start an update ([3ca56030](https://github.com/hanzydev/Fileship/commit/3ca56030))
- **api:** Rate limit inbox uploads per client ([ad27f390](https://github.com/hanzydev/Fileship/commit/ad27f390))
- **desktop:** Stop the drive when the session expires ([ad88f700](https://github.com/hanzydev/Fileship/commit/ad88f700))
- **api:** Disconnect both sessions when signing out while impersonating ([a5549c90](https://github.com/hanzydev/Fileship/commit/a5549c90))
- **api:** Abort a deleted user's pending uploads on their storage ([24afabe3](https://github.com/hanzydev/Fileship/commit/24afabe3))
- **desktop:** Run drive helper events through the sync queue ([4363044e](https://github.com/hanzydev/Fileship/commit/4363044e))
- **desktop:** Reload the drive after a backup restore ([7a5aa215](https://github.com/hanzydev/Fileship/commit/7a5aa215))
- **api:** Trim custom domains before dropping empty ones and validate them ([fda94dc8](https://github.com/hanzydev/Fileship/commit/fda94dc8))
- **api:** Reject empty and UUID inbox slugs and resolve inbox ids first ([9fbc71f1](https://github.com/hanzydev/Fileship/commit/9fbc71f1))
- **desktop:** Keep the pending identity of resumed drive uploads ([8ab79f57](https://github.com/hanzydev/Fileship/commit/8ab79f57))
- **api:** Never cache settings loaded before the last update ([8fa5c78f](https://github.com/hanzydev/Fileship/commit/8fa5c78f))
- **ui:** Show an error with a retry when a list fails to load ([28dc845c](https://github.com/hanzydev/Fileship/commit/28dc845c))
- **ui:** Virtualize the folder lists and the folder file picker ([663ed5e0](https://github.com/hanzydev/Fileship/commit/663ed5e0))
- **api:** Move S3 objects larger than 5 GB with a multipart copy ([61435832](https://github.com/hanzydev/Fileship/commit/61435832))
- **desktop:** Report re-added drive files and rescan after watcher overflows ([80c4a1f7](https://github.com/hanzydev/Fileship/commit/80c4a1f7))
- **api:** Stop retrying thumbnails that cannot be generated on every start ([544a63a8](https://github.com/hanzydev/Fileship/commit/544a63a8))
- **desktop:** Treat files replaced right after a delete as edits ([21792926](https://github.com/hanzydev/Fileship/commit/21792926))
- **ui:** Upload empty files as one empty chunk ([7749051b](https://github.com/hanzydev/Fileship/commit/7749051b))
- **ui:** Keep queued files that only share a name ([a0b80910](https://github.com/hanzydev/Fileship/commit/a0b80910))
- **ui:** End the update dialog when the updater stream stops ([f96222e6](https://github.com/hanzydev/Fileship/commit/f96222e6))
- **api:** Search public folders inside the folder instead of the owner's top matches ([3d174e77](https://github.com/hanzydev/Fileship/commit/3d174e77))
- **api:** Keep the display names of locked files hidden from visitors ([0007ac06](https://github.com/hanzydev/Fileship/commit/0007ac06))
- **ui:** Only follow same-site redirects after login ([1741a55a](https://github.com/hanzydev/Fileship/commit/1741a55a))
- **ui:** Show the white play icon only over video thumbnails ([fd55cc36](https://github.com/hanzydev/Fileship/commit/fd55cc36))
- Count trashed files in stats and show the trashed size ([d699cda1](https://github.com/hanzydev/Fileship/commit/d699cda1))
- **ui:** Stop the stats skeletons and report the error when loading fails ([040077bf](https://github.com/hanzydev/Fileship/commit/040077bf))
- **api:** Include the upload concurrency in public settings and their updates ([8bb84e13](https://github.com/hanzydev/Fileship/commit/8bb84e13))
- **telemetry:** Check failed logins per client first and resolve client IPs from the trusted proxy ([16574a4b](https://github.com/hanzydev/Fileship/commit/16574a4b))
- **telemetry:** Limit how many new instances one client can report per day ([56bfee98](https://github.com/hanzydev/Fileship/commit/56bfee98))
- **telemetry:** Accept reports with missing or unknown category and storage keys ([8dc31781](https://github.com/hanzydev/Fileship/commit/8dc31781))
- **desktop:** Upload overwritten drive files as edits ([7d3bd036](https://github.com/hanzydev/Fileship/commit/7d3bd036))
- **api:** Create a missing inbox instead of failing to save it ([e9ad0a22](https://github.com/hanzydev/Fileship/commit/e9ad0a22))
- **desktop:** Reuse the instance check when logging in ([64d4a7d8](https://github.com/hanzydev/Fileship/commit/64d4a7d8))
- **ui:** Verify before testing a saved storage and starting an update ([eaeddae4](https://github.com/hanzydev/Fileship/commit/eaeddae4))
- **desktop:** Open only http and https links outside the app ([63c24b6e](https://github.com/hanzydev/Fileship/commit/63c24b6e))
- **desktop:** Deny web permissions in the dashboard and login windows ([ea87ef03](https://github.com/hanzydev/Fileship/commit/ea87ef03))
- **desktop:** Store the upload password encrypted ([92c3b78a](https://github.com/hanzydev/Fileship/commit/92c3b78a))
- **desktop:** Finish a recording only once ([19802b64](https://github.com/hanzydev/Fileship/commit/19802b64))
- **desktop:** Wait for the recording upload before installing updates ([d20da5f3](https://github.com/hanzydev/Fileship/commit/d20da5f3))
- **desktop:** Keep captures and recordings saved in the same second ([93848318](https://github.com/hanzydev/Fileship/commit/93848318))
- **desktop:** Keep a recording whose upload failed ([56f2fdf9](https://github.com/hanzydev/Fileship/commit/56f2fdf9))
- **desktop:** Fail the ffmpeg download when the file cannot be written ([245e3d30](https://github.com/hanzydev/Fileship/commit/245e3d30))
- **telemetry:** Show load errors and return to the login on an expired session ([42656974](https://github.com/hanzydev/Fileship/commit/42656974))
- **desktop:** Recover capture overlays that crash, hang or never load ([b7b6b8bc](https://github.com/hanzydev/Fileship/commit/b7b6b8bc))
- **desktop:** Harden the editor, overlay and recorder windows ([5b7439a5](https://github.com/hanzydev/Fileship/commit/5b7439a5))
- **desktop:** Open one dashboard window for quick repeated clicks ([f4b30299](https://github.com/hanzydev/Fileship/commit/f4b30299))
- **desktop:** Refresh the upload limits instead of caching them forever ([6bad0c34](https://github.com/hanzydev/Fileship/commit/6bad0c34))
- **desktop:** Report failures of captures and recorder controls started from pages ([510f0582](https://github.com/hanzydev/Fileship/commit/510f0582))
- **desktop:** Register hotkeys again when a window closes while recording one ([1cddbf63](https://github.com/hanzydev/Fileship/commit/1cddbf63))
- **desktop:** Refresh the tray menu when the update, drive or account state changes ([5dc76f7e](https://github.com/hanzydev/Fileship/commit/5dc76f7e))
- **desktop:** Keep at most one pending update check retry ([c3ec93c2](https://github.com/hanzydev/Fileship/commit/c3ec93c2))
- **ui:** Render skeleton text blocks as divs ([bc4b8690](https://github.com/hanzydev/Fileship/commit/bc4b8690))
- **api:** Ship the AI worker in production builds ([80019129](https://github.com/hanzydev/Fileship/commit/80019129))
- **api:** Tokenize search text for every script ([5ad33043](https://github.com/hanzydev/Fileship/commit/5ad33043))
- **ui:** Animate closing the public folder dialogs ([70d57f8c](https://github.com/hanzydev/Fileship/commit/70d57f8c))
- **desktop:** Take over the drive folder from a stale sync root ([536cb3bf](https://github.com/hanzydev/Fileship/commit/536cb3bf))
- **api:** Fold Unicode file names into ASCII slugs ([a64d70e5](https://github.com/hanzydev/Fileship/commit/a64d70e5))
- **desktop:** Lower the shutter and upload sounds ([a45f5438](https://github.com/hanzydev/Fileship/commit/a45f5438))
- **desktop:** Outline recording areas that span monitors ([a59c2f59](https://github.com/hanzydev/Fileship/commit/a59c2f59))
- **desktop:** Disable DevTools and debugging in packaged builds ([71c75426](https://github.com/hanzydev/Fileship/commit/71c75426))
- **desktop:** Spell FFmpeg with its own casing ([ffe41d51](https://github.com/hanzydev/Fileship/commit/ffe41d51))
- **api:** Install without a built AI package ([ab505a58](https://github.com/hanzydev/Fileship/commit/ab505a58))
- **ui:** Paint the mobile browser bar with the page background ([b9671114](https://github.com/hanzydev/Fileship/commit/b9671114))
- **desktop:** Show toolbar tooltips without lag ([5e2de76a](https://github.com/hanzydev/Fileship/commit/5e2de76a))
- **desktop:** Centre the restart button of the update banner ([a5a9df04](https://github.com/hanzydev/Fileship/commit/a5a9df04))
- **desktop:** Make capture, recording and upload flows behave the same ([7be3f7b5](https://github.com/hanzydev/Fileship/commit/7be3f7b5))
- **desktop:** Align small settings and toolbar details ([7b3507cc](https://github.com/hanzydev/Fileship/commit/7b3507cc))

### 🔥 Performance

- **desktop:** Encode each capture once off the main thread ([4c719614](https://github.com/hanzydev/Fileship/commit/4c719614))
- **desktop:** Create capture overlays on demand ([4b510873](https://github.com/hanzydev/Fileship/commit/4b510873))
- **desktop:** Create the main window only while it is shown ([225e6e38](https://github.com/hanzydev/Fileship/commit/225e6e38))
- **api:** Run the AI models on a worker thread ([9125f133](https://github.com/hanzydev/Fileship/commit/9125f133))

### 💅 Refactors

- **ui:** Migrate dashboard to shadcn ([d33ba2de](https://github.com/hanzydev/Fileship/commit/d33ba2de))
- Use status instead of deprecated statusCode ([0ffcda90](https://github.com/hanzydev/Fileship/commit/0ffcda90))
- **auth:** Use simplewebauthn sendSignal ([e41e38d5](https://github.com/hanzydev/Fileship/commit/e41e38d5))
- **api:** Share user creation and session start ([64972bc2](https://github.com/hanzydev/Fileship/commit/64972bc2))
- ⚠️ Read server config from runtimeConfig ([b9268099](https://github.com/hanzydev/Fileship/commit/b9268099))
- **ui:** Move the design system into a shared Nuxt layer ([7d772599](https://github.com/hanzydev/Fileship/commit/7d772599))
- **desktop:** Capture and window calls through the native addon ([f2e629ca](https://github.com/hanzydev/Fileship/commit/f2e629ca))
- **ui:** Share the bulk id chunking ([fc8788c1](https://github.com/hanzydev/Fileship/commit/fc8788c1))
- **ui:** Inline the folder card into the folders page ([e5c18379](https://github.com/hanzydev/Fileship/commit/e5c18379))
- **ui:** Inline the updater card into the sidebar ([bfee9c29](https://github.com/hanzydev/Fileship/commit/bfee9c29))
- **ui:** Inline the user sheet into the users page ([1b98ca60](https://github.com/hanzydev/Fileship/commit/1b98ca60))
- **ui:** Inline the command palette, verify dialog and upload queue into the dashboard layout ([bce9c825](https://github.com/hanzydev/Fileship/commit/bce9c825))
- **ui:** Move the trend chart into the ui layer ([068e6e1c](https://github.com/hanzydev/Fileship/commit/068e6e1c))
- **ui:** Share the error page through the ui layer ([23527ca0](https://github.com/hanzydev/Fileship/commit/23527ca0))
- **desktop:** Share one MIME map and one notification helper ([44553322](https://github.com/hanzydev/Fileship/commit/44553322))
- **ui:** Move the data types out of the composables ([48958ed7](https://github.com/hanzydev/Fileship/commit/48958ed7))
- **ui:** Use the shared trend chart for the views statistics ([f64a9dbf](https://github.com/hanzydev/Fileship/commit/f64a9dbf))
- **desktop:** Cover every monitor with one capture overlay ([d8ace30f](https://github.com/hanzydev/Fileship/commit/d8ace30f))
- **i18n:** Keep one catalog file per locale ([fcd22234](https://github.com/hanzydev/Fileship/commit/fcd22234))

### 📖 Documentation

- Add AGENTS.md ([c7f22390](https://github.com/hanzydev/Fileship/commit/c7f22390))
- Refresh screenshots ([25eb16a7](https://github.com/hanzydev/Fileship/commit/25eb16a7))
- Refresh documentation for 6.0 ([5e635f39](https://github.com/hanzydev/Fileship/commit/5e635f39))
- Document sessions, upload tokens and media processing ([34dc1699](https://github.com/hanzydev/Fileship/commit/34dc1699))
- Storage backends ([cef7098a](https://github.com/hanzydev/Fileship/commit/cef7098a))
- Correct storage, archive and chunk size details ([1bd800d6](https://github.com/hanzydev/Fileship/commit/1bd800d6))
- Trash and link shortener ([724b998a](https://github.com/hanzydev/Fileship/commit/724b998a))
- AI models and sensitive content ([5e1d474a](https://github.com/hanzydev/Fileship/commit/5e1d474a))
- Model based sensitive content detection ([b66691f5](https://github.com/hanzydev/Fileship/commit/b66691f5))
- Requirements with and without AI ([a4158833](https://github.com/hanzydev/Fileship/commit/a4158833))
- AI presets, models and requirements ([479df587](https://github.com/hanzydev/Fileship/commit/479df587))
- Complete environment variables and the 6.0 upgrade notes ([dc2167f5](https://github.com/hanzydev/Fileship/commit/dc2167f5))
- Shared ui layer, telemetry contract and the telemetry panel ([b3515940](https://github.com/hanzydev/Fileship/commit/b3515940))
- Align the docs with how updates, AI, settings and the monorepo work ([5c84ac14](https://github.com/hanzydev/Fileship/commit/5c84ac14))
- Describe the desktop app ([5dd4f1e6](https://github.com/hanzydev/Fileship/commit/5dd4f1e6))
- Clarify the desktop main window lifecycle ([97b49fda](https://github.com/hanzydev/Fileship/commit/97b49fda))
- **telemetry:** Explain that the saved instance setting overrides the env variable ([f8599edc](https://github.com/hanzydev/Fileship/commit/f8599edc))
- Describe trash in quotas and stats, storage sync grace period and new instance limit ([544ec415](https://github.com/hanzydev/Fileship/commit/544ec415))
- Document the shared error page, trend chart, contained lists and data types ([cad10ee6](https://github.com/hanzydev/Fileship/commit/cad10ee6))
- Add MEMORY.md and PLAN.md for contributors and AI agents ([15b1d35c](https://github.com/hanzydev/Fileship/commit/15b1d35c))
- Describe translations and the Turkish style guide ([30baed6a](https://github.com/hanzydev/Fileship/commit/30baed6a))

### 📦 Build

- Install from the lockfile in the images and keep app .env files out ([c58829d4](https://github.com/hanzydev/Fileship/commit/c58829d4))

### 🤖 CI

- Publish the telemetry panel image ([f5167ef6](https://github.com/hanzydev/Fileship/commit/f5167ef6))
- Build and publish the desktop app on desktop-v tags ([af366579](https://github.com/hanzydev/Fileship/commit/af366579))
- Skip the desktop native builds outside Windows ([38c109ae](https://github.com/hanzydev/Fileship/commit/38c109ae))
- Create the desktop release before electron-builder uploads to it ([6c4e4145](https://github.com/hanzydev/Fileship/commit/6c4e4145))

### 🎨 Styles

- **desktop:** Sort the MIME map imports ([9e2554b1](https://github.com/hanzydev/Fileship/commit/9e2554b1))
- **api:** Format the login rate limit keys ([a1844499](https://github.com/hanzydev/Fileship/commit/a1844499))
- **ui:** Order the skeleton text block attributes ([85628720](https://github.com/hanzydev/Fileship/commit/85628720))

### 🏡 Chore

- **deps:** Upgrade nuxt to 4.6.1 & bump dependencies ([2fccf842](https://github.com/hanzydev/Fileship/commit/2fccf842))
- Remove legacy dashboard code ([f7d42100](https://github.com/hanzydev/Fileship/commit/f7d42100))
- Add cloudflare turnstile types ([c7f32ad3](https://github.com/hanzydev/Fileship/commit/c7f32ad3))
- Prebundle more client dependencies ([5d45b7d4](https://github.com/hanzydev/Fileship/commit/5d45b7d4))
- Remove unused vueuse patch ([2e1bd44d](https://github.com/hanzydev/Fileship/commit/2e1bd44d))
- Prebundle ui dependencies ([1a2b0e3c](https://github.com/hanzydev/Fileship/commit/1a2b0e3c))
- Prebundle chart dependencies ([9aa63dc4](https://github.com/hanzydev/Fileship/commit/9aa63dc4))
- Sign the build assets directory ([6908ce6a](https://github.com/hanzydev/Fileship/commit/6908ce6a))
- Prebundle auth dependencies ([8cec5e88](https://github.com/hanzydev/Fileship/commit/8cec5e88))
- Type check every workspace in pnpm check ([15540447](https://github.com/hanzydev/Fileship/commit/15540447))
- **telemetry-panel:** Remove the legacy import ([fd9ff9d1](https://github.com/hanzydev/Fileship/commit/fd9ff9d1))
- Declare direct dependencies and tidy the workspace ([48a9a0c8](https://github.com/hanzydev/Fileship/commit/48a9a0c8))
- Install only-allow without a prompt in preinstall ([ffd34245](https://github.com/hanzydev/Fileship/commit/ffd34245))
- Remove changelogen ([867b8177](https://github.com/hanzydev/Fileship/commit/867b8177))
- Bump version to 6.0.0 ([3512f6c1](https://github.com/hanzydev/Fileship/commit/3512f6c1))
- **ui:** Drop the socket logs and comments ([15fc3a23](https://github.com/hanzydev/Fileship/commit/15fc3a23))
- Remove the unused folder archive socket event ([ffbc8ce3](https://github.com/hanzydev/Fileship/commit/ffbc8ce3))
- **desktop:** Remove drive helper commands that are never sent ([3f1018c8](https://github.com/hanzydev/Fileship/commit/3f1018c8))
- **desktop:** Remove the unused openExternal and ffmpeg remove channels ([4fde3141](https://github.com/hanzydev/Fileship/commit/4fde3141))
- **ui:** Remove unused shadcn components ([dcb9c35c](https://github.com/hanzydev/Fileship/commit/dcb9c35c))
- **eslint:** Drop the unused ~~ import group ([b49aafcc](https://github.com/hanzydev/Fileship/commit/b49aafcc))
- **desktop:** Version the app 0.0.1 until v6 ships ([93de783c](https://github.com/hanzydev/Fileship/commit/93de783c))
- Format the translated pages and the docs ([4fdf84cc](https://github.com/hanzydev/Fileship/commit/4fdf84cc))
- **desktop:** Release 0.0.2 ([7da3d2b2](https://github.com/hanzydev/Fileship/commit/7da3d2b2))
- **desktop:** Release 0.0.3 ([b1ec1281](https://github.com/hanzydev/Fileship/commit/b1ec1281))
- **i18n:** Write the ellipsis character in every catalog ([d3ee2cb2](https://github.com/hanzydev/Fileship/commit/d3ee2cb2))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.4.4

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.4.3...v5.4.4)

### 🩹 Fixes

- **prisma:** Set default empty array for file piiReasons ([d20ee38](https://github.com/hanzydev/Fileship/commit/d20ee38))
- **upload:** Track chunks with uploadId to prevent missing chunk error ([e7bfe81](https://github.com/hanzydev/Fileship/commit/e7bfe81))

### 📖 Documentation

- Update security policy ([ae2fce9](https://github.com/hanzydev/Fileship/commit/ae2fce9))

### 🏡 Chore

- **deps-dev:** Bump nuxt from 4.5.0 to 4.5.1 ([09c751e](https://github.com/hanzydev/Fileship/commit/09c751e))
- Regenerate lockfile & lint fix ([6c2ae7d](https://github.com/hanzydev/Fileship/commit/6c2ae7d))
- Remove old mcps ([1123edb](https://github.com/hanzydev/Fileship/commit/1123edb))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.4.3

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.4.2...v5.4.3)

### 🚀 Enhancements

- **updater-agent:** Refactor update flow with dynamic version check and healthz ([00005fb](https://github.com/hanzydev/Fileship/commit/00005fb))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.4.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.4.1...v5.4.2)

### 🩹 Fixes

- Resolve infinite recreating state and add healthcheck ([29c60d0](https://github.com/hanzydev/Fileship/commit/29c60d0))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.4.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.4.0...v5.4.1)

### 🩹 Fixes

- **docker:** Add native build dependencies for arm64 updater-agent ([f16df0f](https://github.com/hanzydev/Fileship/commit/f16df0f))
- Fix sharp pipeline bugs, EXIF retention, and file collision risks ([4da7fa5](https://github.com/hanzydev/Fileship/commit/4da7fa5))
- Upgrade sharp to patch known vulnerabilities ([ad88473](https://github.com/hanzydev/Fileship/commit/ad88473))

### 📦 Build

- **docker:** Switch updater-agent base image from alpine to node 24 slim ([257fccb](https://github.com/hanzydev/Fileship/commit/257fccb))
- **docker:** Use docker/compose-bin for updater-agent runner ([7e263ba](https://github.com/hanzydev/Fileship/commit/7e263ba))

### 🏡 Chore

- Disable onnxruntime-node and protobufjs in pnpm-workspace hoisting configuration ([b9b7d7e](https://github.com/hanzydev/Fileship/commit/b9b7d7e))
- Regenerate lockfile ([0a92452](https://github.com/hanzydev/Fileship/commit/0a92452))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.4.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.3.0...v5.4.0)

### 🚀 Enhancements

- Add automatic and one-click update support ([12d85c3](https://github.com/hanzydev/Fileship/commit/12d85c3))

### 🩹 Fixes

- Prevent upload queue state corruption during concurrent paste operations ([292e93e](https://github.com/hanzydev/Fileship/commit/292e93e))
- **sse parser:** Process trailing buffer data on sse stream end ([4a3c837](https://github.com/hanzydev/Fileship/commit/4a3c837))

### 📖 Documentation

- **readme:** Replace aggressive safari rant with modern browser recommendations ([8ec84e0](https://github.com/hanzydev/Fileship/commit/8ec84e0))

### 🏡 Chore

- Generate lock file ([98babaa](https://github.com/hanzydev/Fileship/commit/98babaa))
- Move eslint config to a standalone package ([f18218f](https://github.com/hanzydev/Fileship/commit/f18218f))
- Lint fix ([fa9b7d1](https://github.com/hanzydev/Fileship/commit/fa9b7d1))
- Update sharp to latest version and remove pnpm override ([3019c7d](https://github.com/hanzydev/Fileship/commit/3019c7d))

### 🎨 Styles

- **ui:** Adjust spacing for AreYouSure modal component ([cbee224](https://github.com/hanzydev/Fileship/commit/cbee224))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.3.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.2.0...v5.3.0)

### 🚀 Enhancements

- Implement dashboard-wide auto-copy for drag-and-drop and paste uploads ([890fbf4](https://github.com/hanzydev/Fileship/commit/890fbf4))
- Add inbox feature ([00e73bf](https://github.com/hanzydev/Fileship/commit/00e73bf))
- **backups:** Add inbox support ([9c20d4a](https://github.com/hanzydev/Fileship/commit/9c20d4a))

### 🩹 Fixes

- **folders:** Disable download button while downloading folder ([844a964](https://github.com/hanzydev/Fileship/commit/844a964))
- **archive:** Move storage setItem call to correct position in archive process ([64e9dd9](https://github.com/hanzydev/Fileship/commit/64e9dd9))
- Center the pii detected text ([c47fb1c](https://github.com/hanzydev/Fileship/commit/c47fb1c))
- Handle case with no files to archive in folder ([fdf0c3b](https://github.com/hanzydev/Fileship/commit/fdf0c3b))
- **docker:** Resolve ARM64 build failures by adding missing dependencies ([f28209a](https://github.com/hanzydev/Fileship/commit/f28209a))
- Add username uniqueness validation in user update ([2836203](https://github.com/hanzydev/Fileship/commit/2836203))
- Handle case when there are no files to archive in the folder ([b726284](https://github.com/hanzydev/Fileship/commit/b726284))
- **users:** Delete issue when user added a passkey ([302b990](https://github.com/hanzydev/Fileship/commit/302b990))
- Correct typo in omitted fields for file query ([df2e31a](https://github.com/hanzydev/Fileship/commit/df2e31a))

### 💅 Refactors

- **users:** Enhance impersonation message for clarity ([9973a24](https://github.com/hanzydev/Fileship/commit/9973a24))
- Remove legacy option from useClipboard ([4070b66](https://github.com/hanzydev/Fileship/commit/4070b66))
- **router:** Replace deprecated next() callback with return values ([b0c11d6](https://github.com/hanzydev/Fileship/commit/b0c11d6))
- Remove legacy option from useClipboard" ([d279918](https://github.com/hanzydev/Fileship/commit/d279918))
- **file viewer:** Modernize ambient mode with VueUse composables ([bf8be70](https://github.com/hanzydev/Fileship/commit/bf8be70))
- **file viewer:** Simplify next and prev file navigation logic ([2e6936a](https://github.com/hanzydev/Fileship/commit/2e6936a))
- **socket:** Simplify connection and disconnection log messages ([5784727](https://github.com/hanzydev/Fileship/commit/5784727))
- Bulletproof auto-copy for drag-drop/paste uploads ([ca4fb67](https://github.com/hanzydev/Fileship/commit/ca4fb67))
- **dashboard:** Ensure cross-browser auto-copy for file uploads ([d93c8a1](https://github.com/hanzydev/Fileship/commit/d93c8a1))
- **docker:** Update fileship service to use image instead of build context ([4c390ae](https://github.com/hanzydev/Fileship/commit/4c390ae))
- Simplify conditionals for AI embedding checks ([98ba98e](https://github.com/hanzydev/Fileship/commit/98ba98e))

### 📖 Documentation

- Update README with installation instructions and add docker-compose.dev.yml for local development ([8c289d2](https://github.com/hanzydev/Fileship/commit/8c289d2))
- Update section headers for clarity in installation instructions ([987c448](https://github.com/hanzydev/Fileship/commit/987c448))
- Add 'Inbox' feature to README ([852e974](https://github.com/hanzydev/Fileship/commit/852e974))

### 🏡 Chore

- **deps-dev:** Bump marked from 18.0.0 to 18.0.2 ([eca869d](https://github.com/hanzydev/Fileship/commit/eca869d))
- Add optimizeDeps configuration for improved dependency handling ([a859d4f](https://github.com/hanzydev/Fileship/commit/a859d4f))
- Special thanks to Safari for being the new IE 🍎 ([1cb94d6](https://github.com/hanzydev/Fileship/commit/1cb94d6))
- Lint fix ([53dcb80](https://github.com/hanzydev/Fileship/commit/53dcb80))
- **deps-dev:** Bump turbo from 2.8.20 to 2.9.14 ([477d174](https://github.com/hanzydev/Fileship/commit/477d174))
- **deps-dev:** Bump nuxt from 4.4.2 to 4.4.7 ([a24e3e9](https://github.com/hanzydev/Fileship/commit/a24e3e9))
- Upgrade deps & lint fix ([c570372](https://github.com/hanzydev/Fileship/commit/c570372))
- Move changelogen to workspace root ([e1afd50](https://github.com/hanzydev/Fileship/commit/e1afd50))

### 🎨 Styles

- **PartialFileCard:** Adjust context menu button border-radius to match design ([a617918](https://github.com/hanzydev/Fileship/commit/a617918))
- **apexcharts:** Adjust charts to match design ([1983471](https://github.com/hanzydev/Fileship/commit/1983471))

### 🤖 CI

- Update GitHub Actions to use latest versions ([14da067](https://github.com/hanzydev/Fileship/commit/14da067))
- Add GitHub Actions workflow for multi-arch GHCR publishing ([232e2f6](https://github.com/hanzydev/Fileship/commit/232e2f6))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.2.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.1.1...v5.2.0)

### 🚀 Enhancements

- Add dashboard-wide drag-and-drop and clipboard paste upload functionality ([b0f06d3](https://github.com/hanzydev/Fileship/commit/b0f06d3))
- Add functionality to move selected files to a folder on files page ([b92d979](https://github.com/hanzydev/Fileship/commit/b92d979))
- **upload:** Revamp file upload page UX ([86498c1](https://github.com/hanzydev/Fileship/commit/86498c1))
- **notes:** Add sharing capability and markdown rendering ([b268129](https://github.com/hanzydev/Fileship/commit/b268129))
- **files:** Add text file preview support ([26f7170](https://github.com/hanzydev/Fileship/commit/26f7170))
- **note card:** Add ability to copy note content ([303f496](https://github.com/hanzydev/Fileship/commit/303f496))
- **notes:** Add preview toggle, and copy functionality to public page ([e826530](https://github.com/hanzydev/Fileship/commit/e826530))
- **files:** Enhance link copying logic for text files ([cb2596c](https://github.com/hanzydev/Fileship/commit/cb2596c))
- **folders:** Add ability to directly download entire folder ([4d688f2](https://github.com/hanzydev/Fileship/commit/4d688f2))

### 🩹 Fixes

- Public url protocol handling returns http ([74a0d51](https://github.com/hanzydev/Fileship/commit/74a0d51))
- Add key binding to aiSuggestion for improved rendering ([57c5070](https://github.com/hanzydev/Fileship/commit/57c5070))
- Disable global drag-and-drop and clipboard upload on the upload page ([d86087e](https://github.com/hanzydev/Fileship/commit/d86087e))
- Prevent upload handling on the upload page for pasted files ([fa55f73](https://github.com/hanzydev/Fileship/commit/fa55f73))
- Enable nitroAutoImports in nuxt configuration ([26473d8](https://github.com/hanzydev/Fileship/commit/26473d8))
- Simplify onDrop handler in dashboard upload functionality ([a704d26](https://github.com/hanzydev/Fileship/commit/a704d26))
- Update selection logic to use filtered files instead of all files ([b4e20ff](https://github.com/hanzydev/Fileship/commit/b4e20ff))
- Clamp upload progress percentage to ensure valid range ([f6c1661](https://github.com/hanzydev/Fileship/commit/f6c1661))
- **FileViewer:** Update opacity class for zoomed-in state to improve visual consistency ([3577928](https://github.com/hanzydev/Fileship/commit/3577928))
- **files:** Handle missing ip in view counter ([28887b5](https://github.com/hanzydev/Fileship/commit/28887b5))
- Ensure async execution in backup restore state reset task ([36fa3c3](https://github.com/hanzydev/Fileship/commit/36fa3c3))
- Reduce gzip level to prevent system freeze ([ec5caae](https://github.com/hanzydev/Fileship/commit/ec5caae))
- Reactivity loss in parallel file uploads ([a9f66f4](https://github.com/hanzydev/Fileship/commit/a9f66f4))

### 💅 Refactors

- **readme:** Update proxy headers for improved request handling ([2608a00](https://github.com/hanzydev/Fileship/commit/2608a00))
- Enhance request URL handling with xForwarded headers ([e37c4c9](https://github.com/hanzydev/Fileship/commit/e37c4c9))
- Remove unnecessary module overrides and set shamefully-hoist to true ([16b7151](https://github.com/hanzydev/Fileship/commit/16b7151))
- Stop using $event for improved readability in VerifyMFA component and login page ([a74a236](https://github.com/hanzydev/Fileship/commit/a74a236))
- Update textarea rows computation to clamp line count between 10 and 20 ([a38609f](https://github.com/hanzydev/Fileship/commit/a38609f))
- Rename acting as to impersonation for better terminology ([625c65b](https://github.com/hanzydev/Fileship/commit/625c65b))
- **notes:** Update toast messages ([7e78344](https://github.com/hanzydev/Fileship/commit/7e78344))
- Remove unused imports across various files ([5da9d6f](https://github.com/hanzydev/Fileship/commit/5da9d6f))
- Centralize download logic to prevent pop-up blocking ([fa06c5f](https://github.com/hanzydev/Fileship/commit/fa06c5f))
- Remove sql from text file types ([9f1112e](https://github.com/hanzydev/Fileship/commit/9f1112e))
- Add logging for archived folder action ([be361dd](https://github.com/hanzydev/Fileship/commit/be361dd))

### 🏡 Chore

- Lint fix ([e871a3d](https://github.com/hanzydev/Fileship/commit/e871a3d))
- Lint fix ([c394ce7](https://github.com/hanzydev/Fileship/commit/c394ce7))

### 🎨 Styles

- **file viewer:** Ensure consistent icon sizes ([c725f99](https://github.com/hanzydev/Fileship/commit/c725f99))
- **folder viewer:** Ensure consistent icon sizes ([598e158](https://github.com/hanzydev/Fileship/commit/598e158))
- **markdown renderer:** Adjust list indentation for better alignment ([6a94b3c](https://github.com/hanzydev/Fileship/commit/6a94b3c))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.1.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.1.0...v5.1.1)

### 🩹 Fixes

- **files:** Enforce ownership validation in queries ([b70280c](https://github.com/hanzydev/Fileship/commit/b70280c))
- **users:** Add allowlist to backup restoration to prevent field injection ([300c12d](https://github.com/hanzydev/Fileship/commit/300c12d))
- **session:** Improve session retrieval and error handling for missing sessions ([671456a](https://github.com/hanzydev/Fileship/commit/671456a))
- **session:** Update credential counter after successful passkey verification ([c77abce](https://github.com/hanzydev/Fileship/commit/c77abce))
- **session:** Prevent verification bypass ([a2425a7](https://github.com/hanzydev/Fileship/commit/a2425a7))
- **session:** Add error handling for missing credentials during session verification ([da0ee58](https://github.com/hanzydev/Fileship/commit/da0ee58))
- Secure WebAuthn flow with server-side challenge verification ([115620e](https://github.com/hanzydev/Fileship/commit/115620e))
- Import missing MaybeRef type ([467f2a9](https://github.com/hanzydev/Fileship/commit/467f2a9))
- Update class binding syntax for conditional rendering in SearchBar component ([b9f34cf](https://github.com/hanzydev/Fileship/commit/b9f34cf))
- Hide user limits from api ([c40264d](https://github.com/hanzydev/Fileship/commit/c40264d))

### 💅 Refactors

- Simplify clearStates function by using clearNuxtState composable ([82356d3](https://github.com/hanzydev/Fileship/commit/82356d3))
- Update old tsconfig.json ([9b5e9e8](https://github.com/hanzydev/Fileship/commit/9b5e9e8))
- Update default cron schedule for deleting expired files to every 5 minutes ([4697214](https://github.com/hanzydev/Fileship/commit/4697214))
- Rewrite telemetry collection logic ([45ffaed](https://github.com/hanzydev/Fileship/commit/45ffaed))
- Rename permissionErrors utility to permissions ([dbf04cb](https://github.com/hanzydev/Fileship/commit/dbf04cb))
- Rename runTasks plugin to setup ([7787765](https://github.com/hanzydev/Fileship/commit/7787765))
- Replace unknown class types with ClassValue in component props ([ce1b0a6](https://github.com/hanzydev/Fileship/commit/ce1b0a6))
- Migrate appConfig to runtimeConfig for Nuxt 5 readiness ([1c14b03](https://github.com/hanzydev/Fileship/commit/1c14b03))
- Update import statement for defu to named import ([7c7503a](https://github.com/hanzydev/Fileship/commit/7c7503a))
- Update ai settings api endpoint ([bf8eec4](https://github.com/hanzydev/Fileship/commit/bf8eec4))
- Add missing AI settings data into user update payload ([1b65a5d](https://github.com/hanzydev/Fileship/commit/1b65a5d))

### 📦 Build

- **config:** Enable nuxt v5 compatibility version ([f2c2f27](https://github.com/hanzydev/Fileship/commit/f2c2f27))

### 🏡 Chore

- Upgrade nuxt and other deps ([c6d7e7b](https://github.com/hanzydev/Fileship/commit/c6d7e7b))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v5.1.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v5.0.0...v5.1.0)

### 🚀 Enhancements

- New user AI settings and new AIs ([bd2d4b3](https://github.com/hanzydev/Fileship/commit/bd2d4b3))
- Add ability to disable all ai features globally ([ac4577c](https://github.com/hanzydev/Fileship/commit/ac4577c))

### 🩹 Fixes

- **version:** Add caching key for version check ([9065019](https://github.com/hanzydev/Fileship/commit/9065019))
- **backups:** Check for ongoing backup restoration before proceeding ([bed3a20](https://github.com/hanzydev/Fileship/commit/bed3a20))
- **input:** Update password visibility icon ([9b32e8d](https://github.com/hanzydev/Fileship/commit/9b32e8d))
- **synb jobs:** Pii detection and caption generation aren't synchronized ([08bbe4c](https://github.com/hanzydev/Fileship/commit/08bbe4c))
- Use mtime instead of birthtime for backups (docker compatibility) ([495994b](https://github.com/hanzydev/Fileship/commit/495994b))

### 💅 Refactors

- **search:** Wrap telemetry call in event.waitUntil for proper execution ([f8eb469](https://github.com/hanzydev/Fileship/commit/f8eb469))
- **backups:** Wrap backup creation in event.waitUntil for proper execution ([599f056](https://github.com/hanzydev/Fileship/commit/599f056))
- **backups:** Restructure backup loading process for improved clarity and error handling ([e28e24b](https://github.com/hanzydev/Fileship/commit/e28e24b))
- Use node:timers/promises instead of promise for sleep utility ([7a919e7](https://github.com/hanzydev/Fileship/commit/7a919e7))
- Add fake loader to MFA dialog for password manager compatibility ([e24e63d](https://github.com/hanzydev/Fileship/commit/e24e63d))
- Remove AI search usage telemetry ([8d820fe](https://github.com/hanzydev/Fileship/commit/8d820fe))

### 🏡 Chore

- Upgrade deps ([6a14b24](https://github.com/hanzydev/Fileship/commit/6a14b24))
- Remove deprecated PII types from database migration ([708eab9](https://github.com/hanzydev/Fileship/commit/708eab9))
- Upgrade deps ([e37f619](https://github.com/hanzydev/Fileship/commit/e37f619))
- Update Node.js version requirement to 24.x or higher ([e1569ee](https://github.com/hanzydev/Fileship/commit/e1569ee))
- Add docs for AI features and how to disable them ([97d7941](https://github.com/hanzydev/Fileship/commit/97d7941))

### 🎨 Styles

- **sidebar:** Enhance version display with transition effects ([169124d](https://github.com/hanzydev/Fileship/commit/169124d))
- **backups:** Update progress bar corner radius for better aesthetics ([3026b51](https://github.com/hanzydev/Fileship/commit/3026b51))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>
- Dark Of Lord <mustafaxxxerdem@gmail.com>

## v5.0.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v4.2.1...v5.0.0)

### 🚀 Enhancements

- New file viewer ui ([77e4fad](https://github.com/hanzydev/Fileship/commit/77e4fad))
- **folders:** Add ability to delete its files when deleting a folder ([3cc07fc](https://github.com/hanzydev/Fileship/commit/3cc07fc))
- **EditUser:** Add caption to password input for clarity on leaving it blank ([1a77c26](https://github.com/hanzydev/Fileship/commit/1a77c26))
- Add confirmation modal for user deletion ([ab500e0](https://github.com/hanzydev/Fileship/commit/ab500e0))
- Add MFA verification for log flushing functionality ([b48ea6a](https://github.com/hanzydev/Fileship/commit/b48ea6a))
- Add ability to bulk delete files ([a372e9a](https://github.com/hanzydev/Fileship/commit/a372e9a))
- Add ability to move file to another folder if it already in a folder ([7a35a7f](https://github.com/hanzydev/Fileship/commit/7a35a7f))
- Enhance folder management ([524f7ff](https://github.com/hanzydev/Fileship/commit/524f7ff))
- Add logo to sidebar ([b5429a9](https://github.com/hanzydev/Fileship/commit/b5429a9))
- Update icons for improved visuals ([a990738](https://github.com/hanzydev/Fileship/commit/a990738))

### 🔥 Performance

- **ViewFile:** Optimize data handling with shallowRef for performance improvements ([ccf2f33](https://github.com/hanzydev/Fileship/commit/ccf2f33))

### 🩹 Fixes

- Add dependency for dev task in turbo.json ([7f98ff0](https://github.com/hanzydev/Fileship/commit/7f98ff0))
- Prevent drawing on canvas if dimensions are not set ([af628ce](https://github.com/hanzydev/Fileship/commit/af628ce))
- Update background color syntax for consistency across components ([fa2c8cf](https://github.com/hanzydev/Fileship/commit/fa2c8cf))
- Remove error handling for image compression failure ([812b200](https://github.com/hanzydev/Fileship/commit/812b200))
- Add thumbnail URL handling for video uploads ([8c5e6af](https://github.com/hanzydev/Fileship/commit/8c5e6af))
- Improve error handling for delete operations across BackupCard, FileCard, FolderCard, NoteCard, and ViewNote components ([f56cc68](https://github.com/hanzydev/Fileship/commit/f56cc68))
- **users:** Update file and note count resolution to use stats object ([4de23c7](https://github.com/hanzydev/Fileship/commit/4de23c7))
- **prisma:** Conditionally set datasource based on DATABASE_URL presence ([50603b0](https://github.com/hanzydev/Fileship/commit/50603b0))
- **ViewFile:** Close modal on background click and improve layout for media controls ([9ceda57](https://github.com/hanzydev/Fileship/commit/9ceda57))
- **ViewFile:** Allow smooth scrolling to active thumbnail based on parameter ([ed85e54](https://github.com/hanzydev/Fileship/commit/ed85e54))
- **ViewFile:** Ensure active thumbnail scrolls after container width is set ([2e7c515](https://github.com/hanzydev/Fileship/commit/2e7c515))
- **ViewFile:** Add margin auto to modal for better centering ([101938d](https://github.com/hanzydev/Fileship/commit/101938d))
- **user:** Prevent unauthorized super admin promotion ([a66630d](https://github.com/hanzydev/Fileship/commit/a66630d))
- **user:** Prevent race condition in super admin demotion ([b4f5ce3](https://github.com/hanzydev/Fileship/commit/b4f5ce3))
- Add missing props to user creation and update endpoints ([667d24e](https://github.com/hanzydev/Fileship/commit/667d24e))
- Login view animation ([b04d076](https://github.com/hanzydev/Fileship/commit/b04d076))
- Ensure selection ring only appears for selectable items ([5b72063](https://github.com/hanzydev/Fileship/commit/5b72063))
- Update socket handling to associate files with their respective folders ([28c1b8d](https://github.com/hanzydev/Fileship/commit/28c1b8d))
- Update folder assignment logic for files on folder update ([238b1c6](https://github.com/hanzydev/Fileship/commit/238b1c6))

### 💅 Refactors

- Rename modal click outside handler's ignore property ([4f79367](https://github.com/hanzydev/Fileship/commit/4f79367))
- Update background color opacity for Sidebar and Modal components ([d5ad5b9](https://github.com/hanzydev/Fileship/commit/d5ad5b9))
- Update background color opacity for Sidebar and Modal components" ([f2b6dbf](https://github.com/hanzydev/Fileship/commit/f2b6dbf))
- Update background color opacity for Sidebar and Modal components ([e6e16c3](https://github.com/hanzydev/Fileship/commit/e6e16c3))
- Update success message for file move and take out actions ([0779f32](https://github.com/hanzydev/Fileship/commit/0779f32))
- **socket:** Remove folder:file:add and folder:file:remove events ([005c9c6](https://github.com/hanzydev/Fileship/commit/005c9c6))
- **ViewNote:** Streamline note navigation by consolidating changeNote logic ([ebbd098](https://github.com/hanzydev/Fileship/commit/ebbd098))
- Remove unused buttonClass prop from Pagination component for clarity ([fedbe66](https://github.com/hanzydev/Fileship/commit/fedbe66))
- **auth:** Simplify admin impersonation permission logic ([c7d584a](https://github.com/hanzydev/Fileship/commit/c7d584a))
- Update admin paths ([c3a2e70](https://github.com/hanzydev/Fileship/commit/c3a2e70))
- Remove unnecessary code from folder deletion logic ([92a15f6](https://github.com/hanzydev/Fileship/commit/92a15f6))
- Update socket event names for consistency and clarity ([e03265d](https://github.com/hanzydev/Fileship/commit/e03265d))
- Remove old socket events ([6ebf737](https://github.com/hanzydev/Fileship/commit/6ebf737))
- Remove unused import of nextTick from process ([26b4d50](https://github.com/hanzydev/Fileship/commit/26b4d50))
- Hide 'delete files too' option for empty folders ([095f162](https://github.com/hanzydev/Fileship/commit/095f162))
- Rename folder modal ([534ac10](https://github.com/hanzydev/Fileship/commit/534ac10))
- **backups:** Update upload icon for better visibility ([2c038f6](https://github.com/hanzydev/Fileship/commit/2c038f6))

### 🏡 Chore

- Lint fix ([d97c048](https://github.com/hanzydev/Fileship/commit/d97c048))
- Lint fix ([f9a5fb1](https://github.com/hanzydev/Fileship/commit/f9a5fb1))
- Update turbo version ([eaa42fe](https://github.com/hanzydev/Fileship/commit/eaa42fe))
- Lint fix ([bb141f9](https://github.com/hanzydev/Fileship/commit/bb141f9))

### 🎨 Styles

- Revamp dashboard design ([d6682e9](https://github.com/hanzydev/Fileship/commit/d6682e9))
- Use solar icons ([0f026b0](https://github.com/hanzydev/Fileship/commit/0f026b0))
- Replace 'Quicksand' with 'Nunito Sans' for consistent typography ([ec7e2ff](https://github.com/hanzydev/Fileship/commit/ec7e2ff))
- Update icon from 'solar:pen-bold' to 'solar:pen-2-bold' for consistency across components ([9037ec6](https://github.com/hanzydev/Fileship/commit/9037ec6))
- Adjust margin for consistency in DashboardContent and simplify layout in dashboard ([2f7353a](https://github.com/hanzydev/Fileship/commit/2f7353a))
- Update button and container rounding for consistency across components ([e43036d](https://github.com/hanzydev/Fileship/commit/e43036d))
- Update rounded classes for consistency across FileCard, FolderFiles, SearchBar, and folder view components ([63f2610](https://github.com/hanzydev/Fileship/commit/63f2610))
- Update button rounding and sizes for consistency in ViewFile and ViewNote components ([ff3bf97](https://github.com/hanzydev/Fileship/commit/ff3bf97))
- Standardize padding across various components for consistency ([ecba428](https://github.com/hanzydev/Fileship/commit/ecba428))
- Simplify UiModal properties in EditFile and ViewFile components for consistency ([6a6d959](https://github.com/hanzydev/Fileship/commit/6a6d959))
- Update background color for sonner toast component for improved visibility ([0df6dd9](https://github.com/hanzydev/Fileship/commit/0df6dd9))
- Improve layout and spacing in AreYouSure modal for better readability ([41bc415](https://github.com/hanzydev/Fileship/commit/41bc415))
- Update titles and descriptions in confirmation modals for clarity ([aa72f82](https://github.com/hanzydev/Fileship/commit/aa72f82))
- **ViewFile:** Adjust spacing and layout for file info display ([831421c](https://github.com/hanzydev/Fileship/commit/831421c))
- Enhance button and input component styles with rounded corners ([c6c3c57](https://github.com/hanzydev/Fileship/commit/c6c3c57))
- Add toaster offset for improved notification positioning ([c58dca9](https://github.com/hanzydev/Fileship/commit/c58dca9))
- Update button border radius for improved UI consistency ([ce962d1](https://github.com/hanzydev/Fileship/commit/ce962d1))
- Update header font size for improved responsiveness across dashboard pages ([9d4405d](https://github.com/hanzydev/Fileship/commit/9d4405d))
- Update logo image source for improved resolution ([a38a521](https://github.com/hanzydev/Fileship/commit/a38a521))
- Update responsive design for dashboard components and sidebar ([c99d162](https://github.com/hanzydev/Fileship/commit/c99d162))
- Update button styling and add user avatar to dashboard header ([5ed629f](https://github.com/hanzydev/Fileship/commit/5ed629f))
- Enhance sidebar transitions and improve layout for themes view ([d47d171](https://github.com/hanzydev/Fileship/commit/d47d171))
- Adjust action buttons position ([6700feb](https://github.com/hanzydev/Fileship/commit/6700feb))
- Add rounded corners to action buttons in logs and users pages ([4218634](https://github.com/hanzydev/Fileship/commit/4218634))
- **sidebar:** Improve layout and enhance version display ([507a254](https://github.com/hanzydev/Fileship/commit/507a254))
- **sidebar:** Adjust gap in sidebar layout for improved spacing ([6281962](https://github.com/hanzydev/Fileship/commit/6281962))
- **readme:** Update banner image ([bdb2d18](https://github.com/hanzydev/Fileship/commit/bdb2d18))
- **readme:** Update banner image again ([0b56c5a](https://github.com/hanzydev/Fileship/commit/0b56c5a))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v4.2.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v4.2.0...v4.2.1)

### 🚀 Enhancements

- Add mcp config ([9ab8a52](https://github.com/hanzydev/Fileship/commit/9ab8a52))

### 🩹 Fixes

- Resolve choppy video playback ([ecc3f35](https://github.com/hanzydev/Fileship/commit/ecc3f35))
- Telemetry package exports ([be521f1](https://github.com/hanzydev/Fileship/commit/be521f1))
- Improve file view tracking and handling of max views ([3652ebc](https://github.com/hanzydev/Fileship/commit/3652ebc))
- Enhance range header handling for file streaming ([801c57b](https://github.com/hanzydev/Fileship/commit/801c57b))

### 💅 Refactors

- Remove application/json from DOCUMENT_FILE_MIME_TYPES ([af687ef](https://github.com/hanzydev/Fileship/commit/af687ef))

### 🏡 Chore

- Upgrade prisma to v7 ([3d113cb](https://github.com/hanzydev/Fileship/commit/3d113cb))
- Upgrade deps ([f21636e](https://github.com/hanzydev/Fileship/commit/f21636e))
- Remove vue-sonner patch and add override @nuxt/vite-builder ([a920676](https://github.com/hanzydev/Fileship/commit/a920676))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v4.2.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v4.1.1...v4.2.0)

### 🚀 Enhancements

- Enhance file type filtering and add support for additional file types ([33ba217](https://github.com/hanzydev/Fileship/commit/33ba217))

### 🩹 Fixes

- Correct overlay color values for Fileship theme ([b777d7f](https://github.com/hanzydev/Fileship/commit/b777d7f))

### 💅 Refactors

- Remove old code ([16c5480](https://github.com/hanzydev/Fileship/commit/16c5480))
- Clean up whitespace in file type checks ([e8a6037](https://github.com/hanzydev/Fileship/commit/e8a6037))
- Update search handling and improve file filtering logic ([d20d437](https://github.com/hanzydev/Fileship/commit/d20d437))

### 🎨 Styles

- Update toast component styling for improved appearance ([c632189](https://github.com/hanzydev/Fileship/commit/c632189))
- Add flex-shrink-0 class to settings button for better layout ([1b87bfe](https://github.com/hanzydev/Fileship/commit/1b87bfe))
- Add lt-sm:wfull class to ShareX config button for responsive design ([e2b4ec8](https://github.com/hanzydev/Fileship/commit/e2b4ec8))
- Enhance layout responsiveness in FileTypePicker and upload components ([5fbade7](https://github.com/hanzydev/Fileship/commit/5fbade7))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v4.1.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v4.1.0...v4.1.1)

### 🚀 Enhancements

- Use rust free prisma orm ([e1ccc17](https://github.com/hanzydev/Fileship/commit/e1ccc17))

### 🔥 Performance

- **ai search:** Remove await from telemetry.collectAISearchUsage for better performance ([3af77c2](https://github.com/hanzydev/Fileship/commit/3af77c2))

### 🩹 Fixes

- Ensure .env file exists ([9cc4e72](https://github.com/hanzydev/Fileship/commit/9cc4e72))
- Import UserPermission in createDefaultUser task ([8aa8c2e](https://github.com/hanzydev/Fileship/commit/8aa8c2e))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v4.1.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v4.0.2...v4.1.0)

### 🚀 Enhancements

- ⚠️  Remove shortened urls feature ([0a29383](https://github.com/hanzydev/Fileship/commit/0a29383))
- ⚠️  Remove codes feature and add text file creating ([b46b6d0](https://github.com/hanzydev/Fileship/commit/b46b6d0))

### 🩹 Fixes

- Handle undefined embedding in file update response ([7aae530](https://github.com/hanzydev/Fileship/commit/7aae530))
- **login:** Fix layout spacing ([667ba04](https://github.com/hanzydev/Fileship/commit/667ba04))
- **readme:** Nuxt icon ([13b054a](https://github.com/hanzydev/Fileship/commit/13b054a))

### 💅 Refactors

- Update TOTP input styles for improved accessibility and focus handling ([c7c83b8](https://github.com/hanzydev/Fileship/commit/c7c83b8))
- **login:** Add button to handle passkey login ([732e20a](https://github.com/hanzydev/Fileship/commit/732e20a))
- ⚠️  Use rolldown-vite instead of vite ([9881eb5](https://github.com/hanzydev/Fileship/commit/9881eb5))
- Better backup endpoints ([b23dfc8](https://github.com/hanzydev/Fileship/commit/b23dfc8))

### 🏡 Chore

- Upgrade deps ([a275aeb](https://github.com/hanzydev/Fileship/commit/a275aeb))
- Lint fix ([a1adb61](https://github.com/hanzydev/Fileship/commit/a1adb61))
- Update Node.js version in Dockerfile and README ([728d408](https://github.com/hanzydev/Fileship/commit/728d408))

#### ⚠️ Breaking Changes

- ⚠️  Remove shortened urls feature ([0a29383](https://github.com/hanzydev/Fileship/commit/0a29383))
- ⚠️  Remove codes feature and add text file creating ([b46b6d0](https://github.com/hanzydev/Fileship/commit/b46b6d0))
- ⚠️  Use rolldown-vite instead of vite ([9881eb5](https://github.com/hanzydev/Fileship/commit/9881eb5))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v4.0.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v4.0.1...v4.0.2)

### 💅 Refactors

- Run server file directly instead of running preview command ([9d3d380](https://github.com/hanzydev/Fileship/commit/9d3d380))
- Adjust similarity threshold for ai search mode ([81e5f24](https://github.com/hanzydev/Fileship/commit/81e5f24))
- Clamp progress calculation to ensure percentage is within bounds ([42f4e46](https://github.com/hanzydev/Fileship/commit/42f4e46))
- Adjust similarity threshold for ai search mode again ([e36ff64](https://github.com/hanzydev/Fileship/commit/e36ff64))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v4.0.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v4.0.0...v4.0.1)

### 🚀 Enhancements

- Migrate to turborepo ([84e4544](https://github.com/hanzydev/Fileship/commit/84e4544))
- Add telemetry module ([794fd8e](https://github.com/hanzydev/Fileship/commit/794fd8e))

### 🩹 Fixes

- Unable to upload non-image files ([ba2c3e5](https://github.com/hanzydev/Fileship/commit/ba2c3e5))
- Exclude embedding prop for client-side FileData type ([0240be8](https://github.com/hanzydev/Fileship/commit/0240be8))
- Make file type filter responsive ([c25979e](https://github.com/hanzydev/Fileship/commit/c25979e))
- Docker ([fa6f346](https://github.com/hanzydev/Fileship/commit/fa6f346))
- Build and lint scripts ([9677430](https://github.com/hanzydev/Fileship/commit/9677430))
- Db:prototype script ([a6dabb6](https://github.com/hanzydev/Fileship/commit/a6dabb6))
- **readme:** Fix screenshot ([b8d6c0b](https://github.com/hanzydev/Fileship/commit/b8d6c0b))
- Update embedding processing to handle rejected promises ([1aeefc2](https://github.com/hanzydev/Fileship/commit/1aeefc2))
- Add type annotation for user data fetching ([4a5c000](https://github.com/hanzydev/Fileship/commit/4a5c000))

### 💅 Refactors

- Remove unnecessary class ([6826309](https://github.com/hanzydev/Fileship/commit/6826309))
- Use async/await and promises for file operations in clearTemp and createDirectories tasks ([22e17a4](https://github.com/hanzydev/Fileship/commit/22e17a4))
- Remove deprecated statusMessage property from error responses ([5a98152](https://github.com/hanzydev/Fileship/commit/5a98152))

### 📖 Documentation

- Enhance telemetry documentation for clarity ([4ea88a9](https://github.com/hanzydev/Fileship/commit/4ea88a9))

### 🏡 Chore

- Update README ([899a67b](https://github.com/hanzydev/Fileship/commit/899a67b))
- Lint fix ([f7b4927](https://github.com/hanzydev/Fileship/commit/f7b4927))
- Add preinstall script to enforce pnpm usage ([9a737bd](https://github.com/hanzydev/Fileship/commit/9a737bd))
- Use pnpm workspace ([995e44b](https://github.com/hanzydev/Fileship/commit/995e44b))
- Remove old Dockerfile ([561cb0f](https://github.com/hanzydev/Fileship/commit/561cb0f))
- Update pnpm ([bfa4a09](https://github.com/hanzydev/Fileship/commit/bfa4a09))
- Update deps ([c4aa25b](https://github.com/hanzydev/Fileship/commit/c4aa25b))
- Update to nuxt v4 ([c976f5b](https://github.com/hanzydev/Fileship/commit/c976f5b))
- Update zod ([c3f596b](https://github.com/hanzydev/Fileship/commit/c3f596b))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v4.0.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v3.2.1...v4.0.0)

### 🚀 Enhancements

- Add custom muted colors for better consistency ([335afc4](https://github.com/hanzydev/Fileship/commit/335afc4))
- Add catppuccin themes ([a17e79f](https://github.com/hanzydev/Fileship/commit/a17e79f))
- Add user statistics to user data responses and update related types ([2edb83e](https://github.com/hanzydev/Fileship/commit/2edb83e))
- Integrate Orama search with AI embedding for enhanced semantic search capabilities using OpenAI/CLIP ([39325bc](https://github.com/hanzydev/Fileship/commit/39325bc))

### 🩹 Fixes

- Copy prisma engine files after nitro server is built ([e82ad78](https://github.com/hanzydev/Fileship/commit/e82ad78))
- Error handling in login ([0afa014](https://github.com/hanzydev/Fileship/commit/0afa014))
- Prevent navigation when modal is closed ([62b4ce6](https://github.com/hanzydev/Fileship/commit/62b4ce6))
- Memory leak in generate embeddings task ([9c7c970](https://github.com/hanzydev/Fileship/commit/9c7c970))
- Import Prisma to resolve undefined reference in addToOrama task ([d6e17f2](https://github.com/hanzydev/Fileship/commit/d6e17f2))
- Correct punctuation in success message for stats generation ([0cbda0b](https://github.com/hanzydev/Fileship/commit/0cbda0b))

### 💅 Refactors

- ⚠️  Use nitro's tasks feature ([2287783](https://github.com/hanzydev/Fileship/commit/2287783))
- **prisma:** ⚠️  Use migrations ([69a7937](https://github.com/hanzydev/Fileship/commit/69a7937))
- ⚠️  Overhauled scripts ([3c5f68a](https://github.com/hanzydev/Fileship/commit/3c5f68a))
- Update db:prototype script to use pnpm build:prisma ([465e0fa](https://github.com/hanzydev/Fileship/commit/465e0fa))
- Remove cleanup from build script ([de49934](https://github.com/hanzydev/Fileship/commit/de49934))
- Add framework to nitro config ([23a5764](https://github.com/hanzydev/Fileship/commit/23a5764))
- Better theme loading script ([91230cf](https://github.com/hanzydev/Fileship/commit/91230cf))
- Remove inconsistent themes ([5a51121](https://github.com/hanzydev/Fileship/commit/5a51121))
- Clean up .gitignore and improve package.json scripts ([7ac012a](https://github.com/hanzydev/Fileship/commit/7ac012a))
- Remove embed property from user update response ([b66e910](https://github.com/hanzydev/Fileship/commit/b66e910))
- Use defu to set default user limits in login responses ([d411e51](https://github.com/hanzydev/Fileship/commit/d411e51))
- Streamline limits assignment using defu for default user limits ([0d58711](https://github.com/hanzydev/Fileship/commit/0d58711))
- Add period to success messages in various tasks ([29de489](https://github.com/hanzydev/Fileship/commit/29de489))
- Update currentUser stats on note, url, code, file, and folder changes ([fea6d51](https://github.com/hanzydev/Fileship/commit/fea6d51))
- Omit embedding field from file and folder queries for improved performance ([893bd15](https://github.com/hanzydev/Fileship/commit/893bd15))

### 🏡 Chore

- Remove renovate configuration file ([527a83f](https://github.com/hanzydev/Fileship/commit/527a83f))
- Rebuild pnpm-lock ([40a14cf](https://github.com/hanzydev/Fileship/commit/40a14cf))
- Lint fix ([6fef3fb](https://github.com/hanzydev/Fileship/commit/6fef3fb))
- Update dev script to include prisma build step ([e24b17c](https://github.com/hanzydev/Fileship/commit/e24b17c))

### 🤖 CI

- Use node.js 24 ([d3ef7f8](https://github.com/hanzydev/Fileship/commit/d3ef7f8))

#### ⚠️ Breaking Changes

- ⚠️  Use nitro's tasks feature ([2287783](https://github.com/hanzydev/Fileship/commit/2287783))
- **prisma:** ⚠️  Use migrations ([69a7937](https://github.com/hanzydev/Fileship/commit/69a7937))
- ⚠️  Overhauled scripts ([3c5f68a](https://github.com/hanzydev/Fileship/commit/3c5f68a))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v3.2.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v3.2.0...v3.2.1)

### 🩹 Fixes

- File navigation buttons and folder files background blur visibility ([6fe6480](https://github.com/hanzydev/Fileship/commit/6fe6480))
- Sort folder files ([81c646c](https://github.com/hanzydev/Fileship/commit/81c646c))
- **EditFile:** Background blur visibility ([876162b](https://github.com/hanzydev/Fileship/commit/876162b))
- Font displayed in regular ([f2f8146](https://github.com/hanzydev/Fileship/commit/f2f8146))

### 💅 Refactors

- **ViewFile:** Optimize file list computation by checking modal state ([7d39ab8](https://github.com/hanzydev/Fileship/commit/7d39ab8))
- **FolderFiles:** Hide background ([c6a39a2](https://github.com/hanzydev/Fileship/commit/c6a39a2))
- Regenerate pnpm lock file ([9053498](https://github.com/hanzydev/Fileship/commit/9053498))
- Migration to vue-sonner v2 ([b25af5b](https://github.com/hanzydev/Fileship/commit/b25af5b))
- **unocss:** Use presetWind3 instead of presetUno ([5ce9b6f](https://github.com/hanzydev/Fileship/commit/5ce9b6f))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v3.2.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v3.1.0...v3.2.0)

### 🚀 Enhancements

- Add navigation buttons for file and note modals ([cd162eb](https://github.com/hanzydev/Fileship/commit/cd162eb))
- Ambient mode ([d132c71](https://github.com/hanzydev/Fileship/commit/d132c71))

### 🩹 Fixes

- Backup restoring model can be closed with esc ([8e88384](https://github.com/hanzydev/Fileship/commit/8e88384))
- **files:** Add `url` for ShareX ([2bd56cb](https://github.com/hanzydev/Fileship/commit/2bd56cb))
- **urls:** Remove minimum length constraint for vanity field ([ffd66c8](https://github.com/hanzydev/Fileship/commit/ffd66c8))
- **EditUrl:** Add required attribute to Vanity input ([daf1912](https://github.com/hanzydev/Fileship/commit/daf1912))
- **users:** Prevent non-admin users from updating the superAdmin field ([87d93c1](https://github.com/hanzydev/Fileship/commit/87d93c1))

### 💅 Refactors

- Remove unnecessary _count field from query ([5639f8f](https://github.com/hanzydev/Fileship/commit/5639f8f))
- Add chunked uploads feature to README ([942df27](https://github.com/hanzydev/Fileship/commit/942df27))
- **passkeys:** Improve security by adding userId to query conditions ([43b1a1d](https://github.com/hanzydev/Fileship/commit/43b1a1d))
- **body validation:** Update validation rules and trim inputs for consistency ([7d7c9e5](https://github.com/hanzydev/Fileship/commit/7d7c9e5))
- Update cloned data when original data changes ([02a82b1](https://github.com/hanzydev/Fileship/commit/02a82b1))

### 🎨 Styles

- **ViewNote:** Add cursor-text prop to UiTextArea for better readability ([f0e7191](https://github.com/hanzydev/Fileship/commit/f0e7191))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v3.1.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v3.1.0...v3.1.1)

### 🩹 Fixes

- Backup restoring model can be closed with esc ([8e88384](https://github.com/hanzydev/Fileship/commit/8e88384))
- **files:** Add `url` for ShareX ([2bd56cb](https://github.com/hanzydev/Fileship/commit/2bd56cb))
- **urls:** Remove minimum length constraint for vanity field ([ffd66c8](https://github.com/hanzydev/Fileship/commit/ffd66c8))
- **EditUrl:** Add required attribute to Vanity input ([daf1912](https://github.com/hanzydev/Fileship/commit/daf1912))
- **users:** Prevent non-admin users from updating the superAdmin field ([87d93c1](https://github.com/hanzydev/Fileship/commit/87d93c1))

### 💅 Refactors

- Remove unnecessary _count field from query ([5639f8f](https://github.com/hanzydev/Fileship/commit/5639f8f))
- Add chunked uploads feature to README ([942df27](https://github.com/hanzydev/Fileship/commit/942df27))
- **passkeys:** Improve security by adding userId to query conditions ([43b1a1d](https://github.com/hanzydev/Fileship/commit/43b1a1d))
- **body validation:** Update validation rules and trim inputs for consistency ([7d7c9e5](https://github.com/hanzydev/Fileship/commit/7d7c9e5))

### 🎨 Styles

- **ViewNote:** Add cursor-text prop to UiTextArea for better readability ([f0e7191](https://github.com/hanzydev/Fileship/commit/f0e7191))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v3.1.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v3.0.0...v3.1.0)

### 🚀 Enhancements

- Add an option to enable/disable pwa ([6802aa4](https://github.com/hanzydev/Fileship/commit/6802aa4))
- Dynamic theme colors ([6121a07](https://github.com/hanzydev/Fileship/commit/6121a07))
- **upload:** Enhance file upload progress with speed and ETA ([092a2ef](https://github.com/hanzydev/Fileship/commit/092a2ef))
- Automatic thumbnail generation for videos ([03017e4](https://github.com/hanzydev/Fileship/commit/03017e4))

### 🩹 Fixes

- Increase delay for ensure DOM readiness in verify mfa modal ([d13bf19](https://github.com/hanzydev/Fileship/commit/d13bf19))

### 💅 Refactors

- **VerifyMFA:** Improve height calculation ([b7e38de](https://github.com/hanzydev/Fileship/commit/b7e38de))
- Use vueuse's usePreferredReducedMotion fn ([4e7968a](https://github.com/hanzydev/Fileship/commit/4e7968a))
- Better code block & tabs ([9e8964a](https://github.com/hanzydev/Fileship/commit/9e8964a))
- Use uuid for stats and views ([dec8790](https://github.com/hanzydev/Fileship/commit/dec8790))
- Remove unknown class ([bb1276c](https://github.com/hanzydev/Fileship/commit/bb1276c))
- Use uqr instead of qrcode ([499baa8](https://github.com/hanzydev/Fileship/commit/499baa8))
- **files:** Remove unused url key ([4df4e88](https://github.com/hanzydev/Fileship/commit/4df4e88))
- **backups:** Sort datas ([bf0e499](https://github.com/hanzydev/Fileship/commit/bf0e499))
- Set default chunk size to 25mb ([fa8ca1b](https://github.com/hanzydev/Fileship/commit/fa8ca1b))
- Hide permissions selector if user super admin ([ffbda43](https://github.com/hanzydev/Fileship/commit/ffbda43))
- **files:** Defer delete:file for folder:file:remove event ([356d1db](https://github.com/hanzydev/Fileship/commit/356d1db))
- **socket.io:** Increase ping timeout ([35e55e5](https://github.com/hanzydev/Fileship/commit/35e55e5))
- **backups:** Rewrite backup logic for clarity ([be42f23](https://github.com/hanzydev/Fileship/commit/be42f23))
- Simplify `sendByFilter` calls by directly passing `isAdmin` ([b00c835](https://github.com/hanzydev/Fileship/commit/b00c835))
- Update border radius in ColorPicker ([27cbafb](https://github.com/hanzydev/Fileship/commit/27cbafb))
- Remove old colors ([599df89](https://github.com/hanzydev/Fileship/commit/599df89))
- Simplify filesize function call ([f56fef2](https://github.com/hanzydev/Fileship/commit/f56fef2))
- Remove redundant log and improve directory creation message ([6b2817b](https://github.com/hanzydev/Fileship/commit/6b2817b))

### 🏡 Chore

- Lint fix ([bc5300a](https://github.com/hanzydev/Fileship/commit/bc5300a))
- Lint fix ([b70c342](https://github.com/hanzydev/Fileship/commit/b70c342))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v3.0.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.3.0...v3.0.0)

### 🚀 Enhancements

- **themes:** Make the default theme better and add onyx theme ([7219dd1](https://github.com/hanzydev/Fileship/commit/7219dd1))

### 🩹 Fixes

- Add error handler to url shortening ([53e3748](https://github.com/hanzydev/Fileship/commit/53e3748))
- Prisma ([e3c61ab](https://github.com/hanzydev/Fileship/commit/e3c61ab))

### 💅 Refactors

- Add vanity checking to url editing ([9a1238c](https://github.com/hanzydev/Fileship/commit/9a1238c))
- Correct typo ([61505a8](https://github.com/hanzydev/Fileship/commit/61505a8))
- **ui:** Reduce accent ring usage ([8304e81](https://github.com/hanzydev/Fileship/commit/8304e81))
- **ui:** Update border radius and colors for consistency ([1f5f61c](https://github.com/hanzydev/Fileship/commit/1f5f61c))
- **ui:** Update border radius and colors for consistency ([3384d4c](https://github.com/hanzydev/Fileship/commit/3384d4c))
- Update preview photos ([1d73d58](https://github.com/hanzydev/Fileship/commit/1d73d58))

### 🏡 Chore

- Upgrade deps ([7905e2f](https://github.com/hanzydev/Fileship/commit/7905e2f))
- Lint fix ([993e06f](https://github.com/hanzydev/Fileship/commit/993e06f))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.3.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.2.5...v2.3.0)

### 🩹 Fixes

- Add missing @vueuse/integrations dep ([a81fe4b](https://github.com/hanzydev/Fileship/commit/a81fe4b))
- Docker file ([a469e44](https://github.com/hanzydev/Fileship/commit/a469e44))
- Override unconfig to ^7.3.0 ([a19d18a](https://github.com/hanzydev/Fileship/commit/a19d18a))

### 💅 Refactors

- Reduce ring usage for better UI ([4f3d109](https://github.com/hanzydev/Fileship/commit/4f3d109))
- Reduce ring usage for better UI ([4ceb383](https://github.com/hanzydev/Fileship/commit/4ceb383))

### 🏡 Chore

- Lint fix ([73fcc6a](https://github.com/hanzydev/Fileship/commit/73fcc6a))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.2.5

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.2.4...v2.2.5)

### 💅 Refactors

- Improve file and folder event handling ([19cc1f2](https://github.com/hanzydev/Fileship/commit/19cc1f2))
- Better logging ([7773e53](https://github.com/hanzydev/Fileship/commit/7773e53))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.2.4

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.2.3...v2.2.4)

### 🩹 Fixes

- Files are not deleted if they are missing locally ([956beb1](https://github.com/hanzydev/Fileship/commit/956beb1))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.2.3

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.2.2...v2.2.3)

### 🩹 Fixes

- Overflow bug in dropdown component ([6eeb5b0](https://github.com/hanzydev/Fileship/commit/6eeb5b0))
- Cannot create default user ([61d0c13](https://github.com/hanzydev/Fileship/commit/61d0c13))
- Resetting the language after editing the code ([8d44f23](https://github.com/hanzydev/Fileship/commit/8d44f23))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.2.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.2.1...v2.2.2)

### 🩹 Fixes

- Users cannot be created and edited when form errors are not received ([bf54d97](https://github.com/hanzydev/Fileship/commit/bf54d97))
- Previous state not being cleared when acting as user ([c9004ea](https://github.com/hanzydev/Fileship/commit/c9004ea))

### 🏡 Chore

- Lint fix ([0a61daa](https://github.com/hanzydev/Fileship/commit/0a61daa))
- Lint fix ([33e80b0](https://github.com/hanzydev/Fileship/commit/33e80b0))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.2.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.2.0...v2.2.1)

### 🩹 Fixes

- Cors configuration ([d4e0ee1](https://github.com/hanzydev/Fileship/commit/d4e0ee1))

### 💅 Refactors

- Update caption for max views input fields ([b7aa32b](https://github.com/hanzydev/Fileship/commit/b7aa32b))
- Add missing features to folder page and partial file card ([3e90c7d](https://github.com/hanzydev/Fileship/commit/3e90c7d))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.2.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.1.2...v2.2.0)

### 🚀 Enhancements

- Usernameless passkey login ([e50f3d2](https://github.com/hanzydev/Fileship/commit/e50f3d2))

### 💅 Refactors

- Add loading state to delete button in ViewFile and ViewNote modals ([74d618b](https://github.com/hanzydev/Fileship/commit/74d618b))
- Update authentication terminology in login and TOTP endpoint ([a094e8f](https://github.com/hanzydev/Fileship/commit/a094e8f))
- Remove unnecessary data attribute in VerifyMFA modal ([65493b0](https://github.com/hanzydev/Fileship/commit/65493b0))
- Better author check ([0d04d2d](https://github.com/hanzydev/Fileship/commit/0d04d2d))
- Improve file name handling and compression in file upload ([1100576](https://github.com/hanzydev/Fileship/commit/1100576))
- Sort passkeys ([1e3c147](https://github.com/hanzydev/Fileship/commit/1e3c147))
- Use nuxt's new shared folder feature ([aafca41](https://github.com/hanzydev/Fileship/commit/aafca41))

### 🏡 Chore

- Set print width to 100 in prettier config ([247824e](https://github.com/hanzydev/Fileship/commit/247824e))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.1.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.1.1...v2.1.2)

### 🩹 Fixes

- Parallel uploading ([5162046](https://github.com/hanzydev/Fileship/commit/5162046))
- Log out all known devices ([215d8c0](https://github.com/hanzydev/Fileship/commit/215d8c0))

### 💅 Refactors

- Include IP address in log message ([46cff01](https://github.com/hanzydev/Fileship/commit/46cff01))
- Set flex shrink to 0 on avatar ([b76cef2](https://github.com/hanzydev/Fileship/commit/b76cef2))
- Show spinner when logging out ([1558438](https://github.com/hanzydev/Fileship/commit/1558438))
- Flush all states on logout ([a55c478](https://github.com/hanzydev/Fileship/commit/a55c478))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.1.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.1.0...v2.1.1)

### 🩹 Fixes

- Storage used chart on bigint numbers ([39c7ea4](https://github.com/hanzydev/Fileship/commit/39c7ea4))

### 💅 Refactors

- Remove unnecessary if statement ([8e59133](https://github.com/hanzydev/Fileship/commit/8e59133))
- Sort storage used by user data ([d23d6d4](https://github.com/hanzydev/Fileship/commit/d23d6d4))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.1.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.0.3...v2.1.0)

### 🚀 Enhancements

- **stats:** Storage used by user ([b1093a7](https://github.com/hanzydev/Fileship/commit/b1093a7))

### 💅 Refactors

- Decrease gzip level to 5 for faster backup creation ([2b5c0ee](https://github.com/hanzydev/Fileship/commit/2b5c0ee))
- Better AMOLED theme ([5596e32](https://github.com/hanzydev/Fileship/commit/5596e32))
- Better avatar component ([a515d3e](https://github.com/hanzydev/Fileship/commit/a515d3e))
- Better loading component ([6f9d403](https://github.com/hanzydev/Fileship/commit/6f9d403))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.0.3

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.0.2...v2.0.3)

### 🩹 Fixes

- Verify mfa modal in mobile ([3c5047e](https://github.com/hanzydev/Fileship/commit/3c5047e))

### 💅 Refactors

- Move isMobile and isReducedMotion to  animateCards utility ([b54a1e2](https://github.com/hanzydev/Fileship/commit/b54a1e2))
- Better login page ([5520c0c](https://github.com/hanzydev/Fileship/commit/5520c0c))
- Better cacheing function for version information ([afb6c10](https://github.com/hanzydev/Fileship/commit/afb6c10))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.0.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.0.1...v2.0.2)

### 💅 Refactors

- Better error handling for passkey registration ([7aa7554](https://github.com/hanzydev/Fileship/commit/7aa7554))
- Error handling for verifying with passkey ([1407181](https://github.com/hanzydev/Fileship/commit/1407181))
- Better way to create passkeys in db ([c19e4c4](https://github.com/hanzydev/Fileship/commit/c19e4c4))
- Update readme ([e60050d](https://github.com/hanzydev/Fileship/commit/e60050d))
- Reset selected method to best method if modal is closed ([525c66d](https://github.com/hanzydev/Fileship/commit/525c66d))
- Update passkey register error message ([3e6a5a9](https://github.com/hanzydev/Fileship/commit/3e6a5a9))
- Remove caching for version info ([3d156f9](https://github.com/hanzydev/Fileship/commit/3d156f9))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.0.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v2.0.0...v2.0.1)

### 🩹 Fixes

- **socket:** Passkey name is empty ([1352639](https://github.com/hanzydev/Fileship/commit/1352639))

### 💅 Refactors

- Focus input on login ([adde2c8](https://github.com/hanzydev/Fileship/commit/adde2c8))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v2.0.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.20.1...v2.0.0)

### 🚀 Enhancements

- Passkeys ([d6a29ca](https://github.com/hanzydev/Fileship/commit/d6a29ca))

### 🔥 Performance

- Do not fetch the data again if they are already in ([f96952a](https://github.com/hanzydev/Fileship/commit/f96952a))
- Do not fetch the data again if they are already in ([955f5f6](https://github.com/hanzydev/Fileship/commit/955f5f6))

### 🩹 Fixes

- Embed and direct urls not updating when changing the name of the file or code ([98a02c9](https://github.com/hanzydev/Fileship/commit/98a02c9))

### 💅 Refactors

- Always disable turnstile if there's no turnstile secret and site key ([23d01b6](https://github.com/hanzydev/Fileship/commit/23d01b6))
- Use store for stats ([0f705a3](https://github.com/hanzydev/Fileship/commit/0f705a3))
- Expose forbiddenError to handle 403 errors better ([8e687f2](https://github.com/hanzydev/Fileship/commit/8e687f2))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.20.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.20.0...v1.20.1)

### 🩹 Fixes

- Load the user's theme when acting as a user ([bfadc4f](https://github.com/hanzydev/Fileship/commit/bfadc4f))
- Load the user's theme at login ([2b5451f](https://github.com/hanzydev/Fileship/commit/2b5451f))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.20.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.19.2...v1.20.0)

### 🚀 Enhancements

- Add initial database migration ([1d43de3](https://github.com/hanzydev/Fileship/commit/1d43de3))
- Synced themes ([fe21763](https://github.com/hanzydev/Fileship/commit/fe21763))

### 🩹 Fixes

- Cache duration for version info ([023e21d](https://github.com/hanzydev/Fileship/commit/023e21d))
- **files:** Storage limit control ([be9f824](https://github.com/hanzydev/Fileship/commit/be9f824))

### 💅 Refactors

- **eslint:** Disable vue/require-valid-default-prop ([1f535c8](https://github.com/hanzydev/Fileship/commit/1f535c8))
- **navbar:** Better back button ([5f8a0b7](https://github.com/hanzydev/Fileship/commit/5f8a0b7))
- Better nothing here component ([dccb610](https://github.com/hanzydev/Fileship/commit/dccb610))
- **table:** Hide the ring of nothing here ([174f757](https://github.com/hanzydev/Fileship/commit/174f757))
- Better choose a file input when changing avatar ([09a20e4](https://github.com/hanzydev/Fileship/commit/09a20e4))
- Use variant groups ([1a9ca8c](https://github.com/hanzydev/Fileship/commit/1a9ca8c))
- Do not change theme if already set ([a56ec64](https://github.com/hanzydev/Fileship/commit/a56ec64))
- Remove variant prop from TextArea component and add border ([e4c6881](https://github.com/hanzydev/Fileship/commit/e4c6881))
- Remove current user update log ([be28f9c](https://github.com/hanzydev/Fileship/commit/be28f9c))
- Better button component ([1ba7355](https://github.com/hanzydev/Fileship/commit/1ba7355))

### 🏡 Chore

- Lint fix ([a9c584e](https://github.com/hanzydev/Fileship/commit/a9c584e))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.19.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.19.1...v1.19.2)

### 🔥 Performance

- **folders:** Add only the necessary files to the search algorithm ([0980117](https://github.com/hanzydev/Fileship/commit/0980117))

### 🩹 Fixes

- **socket:** Fix bugs when updating non-store items ([735bc4a](https://github.com/hanzydev/Fileship/commit/735bc4a))
- **socket:** Re-adding already existing files to a folder ([fa3a20e](https://github.com/hanzydev/Fileship/commit/fa3a20e))

### 💅 Refactors

- Ui improvements ([addda20](https://github.com/hanzydev/Fileship/commit/addda20))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.19.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.19.0...v1.19.1)

### 🚀 Enhancements

- **renovate:** Add labels ([65172c2](https://github.com/hanzydev/Fileship/commit/65172c2))

### 🩹 Fixes

- **socket:** Fix legacy code ([ec62618](https://github.com/hanzydev/Fileship/commit/ec62618))
- **socket:** Socket-related issues when updating users not in the store ([07848c0](https://github.com/hanzydev/Fileship/commit/07848c0))

### 💅 Refactors

- **login:** Update old error messages ([e25e8b4](https://github.com/hanzydev/Fileship/commit/e25e8b4))
- **login:** Corrected typo ([bb66673](https://github.com/hanzydev/Fileship/commit/bb66673))
- **totp:** Better verify modal ([19fa875](https://github.com/hanzydev/Fileship/commit/19fa875))
- More understandable and responsive password verify modal ([508a556](https://github.com/hanzydev/Fileship/commit/508a556))
- More responsive totp input ([11df7e9](https://github.com/hanzydev/Fileship/commit/11df7e9))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.19.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.7...v1.19.0)

### 🚀 Enhancements

- **backups:** Backup some user settings ([728f45e](https://github.com/hanzydev/Fileship/commit/728f45e))
- **backups:** Add restore progress feedback ([5ebc568](https://github.com/hanzydev/Fileship/commit/5ebc568))

### 🔥 Performance

- Cache the version information to improve performance and prevent potential errors ([d5ff8ad](https://github.com/hanzydev/Fileship/commit/d5ff8ad))

### 🩹 Fixes

- Typo on view file page ([a7bbe73](https://github.com/hanzydev/Fileship/commit/a7bbe73))
- **permissions:** Cannot read properties of null 'includes' ([e4fdff8](https://github.com/hanzydev/Fileship/commit/e4fdff8))

### 💅 Refactors

- Remove unused validation ([c6621b4](https://github.com/hanzydev/Fileship/commit/c6621b4))
- Use groups on motion-safe ([133cc46](https://github.com/hanzydev/Fileship/commit/133cc46))
- Update some toast messages ([0b4e316](https://github.com/hanzydev/Fileship/commit/0b4e316))
- Regenerate lock file ([12b4fa0](https://github.com/hanzydev/Fileship/commit/12b4fa0))
- **readme:** Add some features not in ([f0e5af2](https://github.com/hanzydev/Fileship/commit/f0e5af2))
- Remove unused process global variable ([ccfd65e](https://github.com/hanzydev/Fileship/commit/ccfd65e))
- Make avatar input better ([eaecdd7](https://github.com/hanzydev/Fileship/commit/eaecdd7))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>
- Emirhan ([@hanzydev](http://github.com/hanzydev))

## v1.18.7

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.5...v1.18.7)

### 💅 Refactors

- Add ring to change avatar input ([6822a43](https://github.com/hanzydev/Fileship/commit/6822a43))
- Better avatar system ([bd4a695](https://github.com/hanzydev/Fileship/commit/bd4a695))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.18.6

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.5...v1.18.6)

### 💅 Refactors

- Adjust height of theme dropdown for compability reasons ([594253e](https://github.com/hanzydev/Fileship/commit/594253e))
- Better permission control ([4f52954](https://github.com/hanzydev/Fileship/commit/4f52954))
- Better socket system ([7bb36d1](https://github.com/hanzydev/Fileship/commit/7bb36d1))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.18.5

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.4...v1.18.5)

### 🩹 Fixes

- Folder selector takes too long when there are many folders ([619f152](https://github.com/hanzydev/Fileship/commit/619f152))
- Allow breaking in folder selectors ([34c858d](https://github.com/hanzydev/Fileship/commit/34c858d))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.18.4

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.3...v1.18.4)

### 🚀 Enhancements

- Add folder selection to upload page ([cb3c17f](https://github.com/hanzydev/Fileship/commit/cb3c17f))

### 🩹 Fixes

- Unocss config ([7135ee2](https://github.com/hanzydev/Fileship/commit/7135ee2))

### 💅 Refactors

- **backups:** Remove extname from uploaded backup log ([a31de49](https://github.com/hanzydev/Fileship/commit/a31de49))
- Use better naming for Impersonate User feature ([a1fbed1](https://github.com/hanzydev/Fileship/commit/a1fbed1))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.18.3

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.2...v1.18.3)

### 🩹 Fixes

- "Cannot read properties of null" bug ([226177c](https://github.com/hanzydev/Fileship/commit/226177c))

### 💅 Refactors

- **renovate:** Disable scheduling ([af869c0](https://github.com/hanzydev/Fileship/commit/af869c0))
- Remove @nuxtjs/turnstile patch ([983acb9](https://github.com/hanzydev/Fileship/commit/983acb9))
- **renovate:** Set rangeStrategy to bump ([ff97fef](https://github.com/hanzydev/Fileship/commit/ff97fef))
- Use windicss breakpoints instead of unocss ([01df63e](https://github.com/hanzydev/Fileship/commit/01df63e))
- A better way to inject theme script ([84c8232](https://github.com/hanzydev/Fileship/commit/84c8232))
- More rounded skeleton lines ([c2a1bf6](https://github.com/hanzydev/Fileship/commit/c2a1bf6))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>
- Emirhan ([@hanzydev](http://github.com/hanzydev))

## v1.18.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.1...v1.18.2)

### 💅 Refactors

- Use [useClipboard](https://vueuse.org/core/useClipboard/) for better compability ([7b3341e](https://github.com/hanzydev/Fileship/commit/7b3341e))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.18.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.18.0...v1.18.1)

### 🚀 Enhancements

- Websocket support for embed config and domains ([ebd5602](https://github.com/hanzydev/Fileship/commit/ebd5602))

### 🩹 Fixes

- URL is empty error on ShareX ([aab41e8](https://github.com/hanzydev/Fileship/commit/aab41e8))
- Align of texts on mobile ([6b424d3](https://github.com/hanzydev/Fileship/commit/6b424d3))

### 💅 Refactors

- Create a util to create public urls more easily ([15e7be9](https://github.com/hanzydev/Fileship/commit/15e7be9))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.18.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.17.1...v1.18.0)

### 🚀 Enhancements

- Format numbers ([d093c13](https://github.com/hanzydev/Fileship/commit/d093c13))
- Use domains everywhere ([40f7223](https://github.com/hanzydev/Fileship/commit/40f7223))

### 🔥 Performance

- Fix double fetching of user data when retrieving domain names ([10d7a7b](https://github.com/hanzydev/Fileship/commit/10d7a7b))
- Significantly reduce the size of logs ([d63669d](https://github.com/hanzydev/Fileship/commit/d63669d))

### 🩹 Fixes

- Some types ([d0740cd](https://github.com/hanzydev/Fileship/commit/d0740cd))
- Chain type ([6aa7863](https://github.com/hanzydev/Fileship/commit/6aa7863))

### 💅 Refactors

- Better user route groups ([544cacc](https://github.com/hanzydev/Fileship/commit/544cacc))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.17.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.17.0...v1.17.1)

### 🔥 Performance

- Disable card animations in mobile ([c5c48e7](https://github.com/hanzydev/Fileship/commit/c5c48e7))

### 🩹 Fixes

- Turnstile type error ([437dec6](https://github.com/hanzydev/Fileship/commit/437dec6))
- Target is readonly warning ([6d81254](https://github.com/hanzydev/Fileship/commit/6d81254))
- Cards do not appear in reduced motion ([04afb01](https://github.com/hanzydev/Fileship/commit/04afb01))

### 💅 Refactors

- **renovate:** Enable dependency dashboard ([a58fb80](https://github.com/hanzydev/Fileship/commit/a58fb80))
- **renovate:** Require approval for major updates ([0022586](https://github.com/hanzydev/Fileship/commit/0022586))
- **renovate:** Enable scheduling ([7425f3b](https://github.com/hanzydev/Fileship/commit/7425f3b))
- Remove reproduction textarea in bug report template ([e8e358b](https://github.com/hanzydev/Fileship/commit/e8e358b))
- Shorten animation durations ([ea0b28c](https://github.com/hanzydev/Fileship/commit/ea0b28c))

### 🏡 Chore

- **deps-dev:** Bump @nuxt/eslint from 0.5.6 to 0.5.7 ([eea542d](https://github.com/hanzydev/Fileship/commit/eea542d))
- **deps-dev:** Bump nuxt from 3.13.1 to 3.13.2 ([976ef01](https://github.com/hanzydev/Fileship/commit/976ef01))
- **deps-dev:** Bump @nuxt/fonts from 0.7.2 to 0.8.0 ([293ece0](https://github.com/hanzydev/Fileship/commit/293ece0))
- Bump deps ([feae2f2](https://github.com/hanzydev/Fileship/commit/feae2f2))
- Remove dependabot ([49e1376](https://github.com/hanzydev/Fileship/commit/49e1376))
- Lint fix ([51cc34e](https://github.com/hanzydev/Fileship/commit/51cc34e))
- Update renovate config ([7e20507](https://github.com/hanzydev/Fileship/commit/7e20507))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>
- Emirhan ([@hanzydev](http://github.com/hanzydev))

## v1.17.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.16.0...v1.17.0)

### 🚀 Enhancements

- Logs are now flushable ([a98edab](https://github.com/hanzydev/Fileship/commit/a98edab))

### 🔥 Performance

- Shorten and performant session verification ([82bd8b5](https://github.com/hanzydev/Fileship/commit/82bd8b5))
- Skeleton loading ([15d53fa](https://github.com/hanzydev/Fileship/commit/15d53fa))

### 🩹 Fixes

- **pagination:** Resolve issue where pagination breaks on data change ([e6f7191](https://github.com/hanzydev/Fileship/commit/e6f7191))
- Force the error information to red when a file gives an error while uploading ([29e84f3](https://github.com/hanzydev/Fileship/commit/29e84f3))
- Target is readonly warning ([d8a43f3](https://github.com/hanzydev/Fileship/commit/d8a43f3))

### 💅 Refactors

- Increase skeleton count ([8b879d4](https://github.com/hanzydev/Fileship/commit/8b879d4))
- A better way to upload files ([188a1e2](https://github.com/hanzydev/Fileship/commit/188a1e2))
- Change class names from mlauto to mla ([d780456](https://github.com/hanzydev/Fileship/commit/d780456))
- Redirect to the current release when clicking on the release information ([f762751](https://github.com/hanzydev/Fileship/commit/f762751))
- Update conditional class name ([8923217](https://github.com/hanzydev/Fileship/commit/8923217))
- Move css imports to nuxt config ([c0ee6df](https://github.com/hanzydev/Fileship/commit/c0ee6df))
- Show spinner when deleting files in action buttons ([8274454](https://github.com/hanzydev/Fileship/commit/8274454))
- Rename Actions to Quick Actions ([c05919c](https://github.com/hanzydev/Fileship/commit/c05919c))
- **AreYouSure:** Improve component logic and event naming ([6c9f06d](https://github.com/hanzydev/Fileship/commit/6c9f06d))

### 🏡 Chore

- Lint fix ([fa8b086](https://github.com/hanzydev/Fileship/commit/fa8b086))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.16.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.15.0...v1.16.0)

### 🚀 Enhancements

- Pwa ([ba77686](https://github.com/hanzydev/Fileship/commit/ba77686))

### 🩹 Fixes

- Sidebar items margin ([b87ac62](https://github.com/hanzydev/Fileship/commit/b87ac62))
- Aria-label attributes ([f014905](https://github.com/hanzydev/Fileship/commit/f014905))
- Prevent entry animation from not playing when a file is uploaded after no file was present ([a50c0b6](https://github.com/hanzydev/Fileship/commit/a50c0b6))

### 💅 Refactors

- Update pwa assets ([d10bd02](https://github.com/hanzydev/Fileship/commit/d10bd02))
- Add preview screenshots to manifest ([e7a5646](https://github.com/hanzydev/Fileship/commit/e7a5646))
- Remove enter and leave animations from recent files ([e4482c4](https://github.com/hanzydev/Fileship/commit/e4482c4))
- Disable paddings in dropdown ([a2e03e6](https://github.com/hanzydev/Fileship/commit/a2e03e6))
- Prevent contextmenu event in dropdown ([3dcacea](https://github.com/hanzydev/Fileship/commit/3dcacea))

### 🏡 Chore

- Remove old nuxt-icon package ([3f8ec04](https://github.com/hanzydev/Fileship/commit/3f8ec04))
- **deps-dev:** Bump marked from 14.1.1 to 14.1.2 ([5167e88](https://github.com/hanzydev/Fileship/commit/5167e88))
- Upgrade deps ([533f91f](https://github.com/hanzydev/Fileship/commit/533f91f))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.15.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.14.1...v1.15.0)

### 🚀 Enhancements

- Add login button to 2fa verification ([29f324e](https://github.com/hanzydev/Fileship/commit/29f324e))
- Add transition to Progress component ([8058322](https://github.com/hanzydev/Fileship/commit/8058322))
- Card animations ([535c247](https://github.com/hanzydev/Fileship/commit/535c247))

### 🩹 Fixes

- @iconify-json/mdi error ([1fff0be](https://github.com/hanzydev/Fileship/commit/1fff0be))
- Files cannot be uploaded ([89a89a7](https://github.com/hanzydev/Fileship/commit/89a89a7))
- Cannot minimize the full screen image ([e5b08dc](https://github.com/hanzydev/Fileship/commit/e5b08dc))

### 💅 Refactors

- Make turnstile size flexible ([f6412f2](https://github.com/hanzydev/Fileship/commit/f6412f2))
- Use vue 3.5 features ([d249d78](https://github.com/hanzydev/Fileship/commit/d249d78))
- Update sameSite attribute for cookies to 'lax' ([df5bc58](https://github.com/hanzydev/Fileship/commit/df5bc58))
- Update file type filter paddings ([240f328](https://github.com/hanzydev/Fileship/commit/240f328))
- Update gap in stat cards ([3ac1bc4](https://github.com/hanzydev/Fileship/commit/3ac1bc4))

### 🏡 Chore

- Update dev script to remove -o flag ([7aa5d82](https://github.com/hanzydev/Fileship/commit/7aa5d82))
- Upgrade deps ([edf4f58](https://github.com/hanzydev/Fileship/commit/edf4f58))
- Lint fix ([2948354](https://github.com/hanzydev/Fileship/commit/2948354))
- Lint fix ([7550a19](https://github.com/hanzydev/Fileship/commit/7550a19))
- Type fixes ([c88ce23](https://github.com/hanzydev/Fileship/commit/c88ce23))
- Upgrade deps ([828be55](https://github.com/hanzydev/Fileship/commit/828be55))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.14.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.14.0...v1.14.1)

### 🩹 Fixes

- Extra gap in select menus ([cc66041](https://github.com/hanzydev/Fileship/commit/cc66041))
- Dependabot config ([e2f5d4f](https://github.com/hanzydev/Fileship/commit/e2f5d4f))
- Expander icon animation ([635c038](https://github.com/hanzydev/Fileship/commit/635c038))

### 💅 Refactors

- Group routes wisely ([18bcfaa](https://github.com/hanzydev/Fileship/commit/18bcfaa))
- Use @nuxt/icon instead of nuxt-icon ([8b7e1ff](https://github.com/hanzydev/Fileship/commit/8b7e1ff))
- Better loading indicator ([fd1818a](https://github.com/hanzydev/Fileship/commit/fd1818a))

### 🏡 Chore

- **deps-dev:** Bump @vueuse/nuxt from 11.0.1 to 11.0.3 ([a4eb5b7](https://github.com/hanzydev/Fileship/commit/a4eb5b7))
- **deps-dev:** Bump prisma from 5.18.0 to 5.19.0 ([c02368c](https://github.com/hanzydev/Fileship/commit/c02368c))
- **deps-dev:** Bump @prisma/client from 5.18.0 to 5.19.0 ([010577a](https://github.com/hanzydev/Fileship/commit/010577a))
- **deps-dev:** Bump eslint from 9.9.0 to 9.9.1 ([0f8bd4c](https://github.com/hanzydev/Fileship/commit/0f8bd4c))
- **deps-dev:** Bump @unocss/eslint-config from 0.62.2 to 0.62.3 ([1a75a2e](https://github.com/hanzydev/Fileship/commit/1a75a2e))
- **deps-dev:** Bump @vueuse/core from 11.0.1 to 11.0.3 ([29e441f](https://github.com/hanzydev/Fileship/commit/29e441f))
- **deps-dev:** Bump apexcharts from 3.52.0 to 3.53.0 ([98292b2](https://github.com/hanzydev/Fileship/commit/98292b2))
- Lint fix ([66c08c1](https://github.com/hanzydev/Fileship/commit/66c08c1))
- Upgrade deps ([ac62e8f](https://github.com/hanzydev/Fileship/commit/ac62e8f))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.14.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.13.0...v1.14.0)

### 🚀 Enhancements

- Improve number input compability ([990afd2](https://github.com/hanzydev/Fileship/commit/990afd2))

### 🩹 Fixes

- Prevent dropdown from overflowing off-screen ([c547695](https://github.com/hanzydev/Fileship/commit/c547695))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.13.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.12.0...v1.13.0)

### 🚀 Enhancements

- Use route groups ([0f8ecc4](https://github.com/hanzydev/Fileship/commit/0f8ecc4))

### 💅 Refactors

- Fix IP address assignment in createLog utility ([e67d2cb](https://github.com/hanzydev/Fileship/commit/e67d2cb))

### 🏡 Chore

- Upgrade nuxt ([e41ef90](https://github.com/hanzydev/Fileship/commit/e41ef90))
- Upgrade dependencies ([10615b2](https://github.com/hanzydev/Fileship/commit/10615b2))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.12.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.11.0...v1.12.0)

### 🚀 Enhancements

- Add option to remove exif data from images during upload ([f3b0c9c](https://github.com/hanzydev/Fileship/commit/f3b0c9c))
- Send generated logs to console ([2baa9d6](https://github.com/hanzydev/Fileship/commit/2baa9d6))
- Add logs to server plugins ([9a35fce](https://github.com/hanzydev/Fileship/commit/9a35fce))

### 🩹 Fixes

- "itemsPerPage" failed: target is readonly. ([b1d3e92](https://github.com/hanzydev/Fileship/commit/b1d3e92))

### 💅 Refactors

- Better log messages ([8470222](https://github.com/hanzydev/Fileship/commit/8470222))
- Set default value of DELETE_EXPIRED_INTERVAL to 30 seconds ([f591fa8](https://github.com/hanzydev/Fileship/commit/f591fa8))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.11.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.10.0...v1.11.0)

### 🚀 Enhancements

- Better solarized dark and material dark theme ([1a63f58](https://github.com/hanzydev/Fileship/commit/1a63f58))
- Better version information ([301ecf8](https://github.com/hanzydev/Fileship/commit/301ecf8))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.10.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.9.3...v1.10.0)

### 🚀 Enhancements

- Use fuzzy-search for better experience ([b088a56](https://github.com/hanzydev/Fileship/commit/b088a56))

### 🩹 Fixes

- Not being deleted after reaching the maximum number of views ([ddf3005](https://github.com/hanzydev/Fileship/commit/ddf3005))

### 🏡 Chore

- Update dependencies ([aff45b1](https://github.com/hanzydev/Fileship/commit/aff45b1))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.9.3

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.9.2...v1.9.3)

### 🩹 Fixes

- Logs not showing ([58f0c1d](https://github.com/hanzydev/Fileship/commit/58f0c1d))

### 🏡 Chore

- Add aria-label to buttons for accessibility ([a5de973](https://github.com/hanzydev/Fileship/commit/a5de973))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.9.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.9.1...v1.9.2)

### 🩹 Fixes

- Old color names ([eb97e8d](https://github.com/hanzydev/Fileship/commit/eb97e8d))

### 💅 Refactors

- Make the clipboard icon white ([c489621](https://github.com/hanzydev/Fileship/commit/c489621))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.9.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.9.0...v1.9.1)

### 💅 Refactors

- Use avatar component fully everywhere ([559de4e](https://github.com/hanzydev/Fileship/commit/559de4e))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.9.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.8.1...v1.9.0)

### 🚀 Enhancements

- Better avatar component with placeholder ([fb61129](https://github.com/hanzydev/Fileship/commit/fb61129))

### 💅 Refactors

- Revert lock file again ([3eb1b01](https://github.com/hanzydev/Fileship/commit/3eb1b01))
- Disable experimental reactive props destructure ([bea7fdb](https://github.com/hanzydev/Fileship/commit/bea7fdb))

### 🏡 Chore

- Update deps ([9a87e0c](https://github.com/hanzydev/Fileship/commit/9a87e0c))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.8.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.8.0...v1.8.1)

### 🩹 Fixes

- Background of the type filter ([ea3e621](https://github.com/hanzydev/Fileship/commit/ea3e621))
- Dropzone not showing up ([fe625ee](https://github.com/hanzydev/Fileship/commit/fe625ee))

### 💅 Refactors

- Recreate pnpm-lock ([47459c7](https://github.com/hanzydev/Fileship/commit/47459c7))
- Fix old color names ([6040ffa](https://github.com/hanzydev/Fileship/commit/6040ffa))
- Add check icon to selected item in pickers ([bbe0ce6](https://github.com/hanzydev/Fileship/commit/bbe0ce6))
- Revert old lock file ([ea98848](https://github.com/hanzydev/Fileship/commit/ea98848))

### 🏡 Chore

- Upgrade deps ([094454f](https://github.com/hanzydev/Fileship/commit/094454f))
- Lint fix ([49fb3ea](https://github.com/hanzydev/Fileship/commit/49fb3ea))
- Type fixes ([f4b629e](https://github.com/hanzydev/Fileship/commit/f4b629e))
- Revert "upgrade deps" ([99a9de2](https://github.com/hanzydev/Fileship/commit/99a9de2))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.8.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.7.0...v1.8.0)

### 🚀 Enhancements

- Add more themes ([03d8a2c](https://github.com/hanzydev/Fileship/commit/03d8a2c))

### 💅 Refactors

- Update NoteCard and ViewFile modals with improved UI and functionality ([b94fd1b](https://github.com/hanzydev/Fileship/commit/b94fd1b))
- Add loading state to delete button in NoteCard and ViewFile modals ([6428c05](https://github.com/hanzydev/Fileship/commit/6428c05))
- Add screenshot to README ([ebee96a](https://github.com/hanzydev/Fileship/commit/ebee96a))
- Update screenshot image ([5905f72](https://github.com/hanzydev/Fileship/commit/5905f72))
- Update screenshot image again ([728b6cc](https://github.com/hanzydev/Fileship/commit/728b6cc))
- Improve mobile responsiveness in NoteCard and ViewFile modals ([73b3db3](https://github.com/hanzydev/Fileship/commit/73b3db3))
- Update copyright notice in LICENSE ([dd45c48](https://github.com/hanzydev/Fileship/commit/dd45c48))
- Add multi-user support with roles to the readme Features section ([d90f2f1](https://github.com/hanzydev/Fileship/commit/d90f2f1))
- A better update guide for Docker in README ([3628470](https://github.com/hanzydev/Fileship/commit/3628470))
- Add custom domains to the README Features section ([4c5d9bc](https://github.com/hanzydev/Fileship/commit/4c5d9bc))
- Improve file card interactivity and view modal behavior ([32e0f84](https://github.com/hanzydev/Fileship/commit/32e0f84))
- Better if statement ([33f6baf](https://github.com/hanzydev/Fileship/commit/33f6baf))
- Update color names for better dev experience ([5b565b4](https://github.com/hanzydev/Fileship/commit/5b565b4))
- Add spaces to theme names ([0a31667](https://github.com/hanzydev/Fileship/commit/0a31667))
- Update features section in README ([4f52c10](https://github.com/hanzydev/Fileship/commit/4f52c10))

### 🏡 Chore

- Lint fix ([d239258](https://github.com/hanzydev/Fileship/commit/d239258))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>
- Nazım Sarp Tekbaş <sarp@sarptekbas.com>

## v1.7.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.6.1...v1.7.0)

### 🚀 Enhancements

- Add play icon for videos ([937cc31](https://github.com/hanzydev/Fileship/commit/937cc31))

### 🩹 Fixes

- Nothing here bug on main page ([9fd8ac5](https://github.com/hanzydev/Fileship/commit/9fd8ac5))

### 💅 Refactors

- Improve ViewFile modal layout and functionality ([867dd77](https://github.com/hanzydev/Fileship/commit/867dd77))
- Improve NoteCard and ViewFile modals ([6020253](https://github.com/hanzydev/Fileship/commit/6020253))

### 🏡 Chore

- Lint fix ([86dab02](https://github.com/hanzydev/Fileship/commit/86dab02))

### ❤️ Contributors

- Nazım Sarp Tekbaş <sarp@sarptekbas.com>
- Hanzydev <contact@hanzy.dev>

## v1.6.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.6.0...v1.6.1)

### 🩹 Fixes

- Type fixes ([8fce8d0](https://github.com/hanzydev/Fileship/commit/8fce8d0))
- Folder handling in socket ([cdad39c](https://github.com/hanzydev/Fileship/commit/cdad39c))

### 💅 Refactors

- Sort files in folder ([2750ba4](https://github.com/hanzydev/Fileship/commit/2750ba4))

### 🏡 Chore

- Update Fileship instructions for updating ([2f749a0](https://github.com/hanzydev/Fileship/commit/2f749a0))
- How did this get here???? ([dc55648](https://github.com/hanzydev/Fileship/commit/dc55648))
- Lint fix ([5afe7c1](https://github.com/hanzydev/Fileship/commit/5afe7c1))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.6.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.5.0...v1.6.0)

### 🚀 Enhancements

- Add fullscreen functionality to image in ViewFile modal ([a39f4e7](https://github.com/hanzydev/Fileship/commit/a39f4e7))
- Add file type filter functionality to folder ([4f50630](https://github.com/hanzydev/Fileship/commit/4f50630))

### 💅 Refactors

- Remove unused code for dropdown ([ceebe40](https://github.com/hanzydev/Fileship/commit/ceebe40))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.5.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.4.0...v1.5.0)

### 🚀 Enhancements

- If the folder is not public, keep the files in it safe ([e647024](https://github.com/hanzydev/Fileship/commit/e647024))
- Add a check mark above the selected files ([5a0708a](https://github.com/hanzydev/Fileship/commit/5a0708a))
- Make slide size better ([378a43b](https://github.com/hanzydev/Fileship/commit/378a43b))
- Add touch event handling for iOS context menu in Dropdown component ([d57bb4e](https://github.com/hanzydev/Fileship/commit/d57bb4e))
- Make selected files more distinct ([f9482fa](https://github.com/hanzydev/Fileship/commit/f9482fa))
- Domains support ([8cce1f2](https://github.com/hanzydev/Fileship/commit/8cce1f2))
- Add log level functionality ([347a06f](https://github.com/hanzydev/Fileship/commit/347a06f))

### 💅 Refactors

- Update error message ([acd65b2](https://github.com/hanzydev/Fileship/commit/acd65b2))
- Take up the vanity ([66d7807](https://github.com/hanzydev/Fileship/commit/66d7807))
- Grammar correction ([d470a16](https://github.com/hanzydev/Fileship/commit/d470a16))
- Grammar correction ([039c63b](https://github.com/hanzydev/Fileship/commit/039c63b))
- Reorganise api routes ([a0b0d94](https://github.com/hanzydev/Fileship/commit/a0b0d94))

### 🏡 Chore

- Update caption for domains input field ([a618fa9](https://github.com/hanzydev/Fileship/commit/a618fa9))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.4.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.3.0...v1.4.0)

### 🚀 Enhancements

- Add session verification when deleting a user ([71dbcb2](https://github.com/hanzydev/Fileship/commit/71dbcb2))
- Add session verification when creating an admin user ([ca63312](https://github.com/hanzydev/Fileship/commit/ca63312))
- Add session validation when switching to a user ([e923256](https://github.com/hanzydev/Fileship/commit/e923256))
- Show the verification modal when creating, editing, deleting and switching users in the client ([8e7d69c](https://github.com/hanzydev/Fileship/commit/8e7d69c))
- **readme:** Add default credentials section ([115895c](https://github.com/hanzydev/Fileship/commit/115895c))

### 🩹 Fixes

- Solve the problem of non-admin users not being able to edit their information ([88b96de](https://github.com/hanzydev/Fileship/commit/88b96de))
- Solve the gap problem on the users page ([2d68433](https://github.com/hanzydev/Fileship/commit/2d68433))
- Disable button when file upload is in progress ([b1b64ea](https://github.com/hanzydev/Fileship/commit/b1b64ea))
- Skip to the next one when there is no file to upload ([3bb84a5](https://github.com/hanzydev/Fileship/commit/3bb84a5))
- Solve the problem of throwing errors if ipinfo.io is blocked ([4e1c982](https://github.com/hanzydev/Fileship/commit/4e1c982))
- Fix session validation not working when editing user ([3082ebc](https://github.com/hanzydev/Fileship/commit/3082ebc))
- Update logout functionality to clear adminSessionId cookie properly ([144a14f](https://github.com/hanzydev/Fileship/commit/144a14f))

### 💅 Refactors

- Switch to ip-api.com ([1ce56e7](https://github.com/hanzydev/Fileship/commit/1ce56e7))
- Update default user username to 'admin' ([f330656](https://github.com/hanzydev/Fileship/commit/f330656))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.3.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.8...v1.3.0)

### 🚀 Enhancements

- Make search bar component more dynamic ([5ce2794](https://github.com/hanzydev/Fileship/commit/5ce2794))
- Type filtering ([7667e12](https://github.com/hanzydev/Fileship/commit/7667e12))
- Make type filtering responsive ([0bfef78](https://github.com/hanzydev/Fileship/commit/0bfef78))

### 🩹 Fixes

- Search bar focus style ([0761fdf](https://github.com/hanzydev/Fileship/commit/0761fdf))

### 🏡 Chore

- Update deps ([8ae3bed](https://github.com/hanzydev/Fileship/commit/8ae3bed))

### 🎨 Styles

- Thin the rings ([c2f8148](https://github.com/hanzydev/Fileship/commit/c2f8148))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.8

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.7...v1.2.8)

### 🩹 Fixes

- Cannot read properties of undefined (reading 'tag_name') error ([0f7b2ab](https://github.com/hanzydev/Fileship/commit/0f7b2ab))

### 💅 Refactors

- Update Sidebar conditional rendering logic for responsive design ([ea061b4](https://github.com/hanzydev/Fileship/commit/ea061b4))
- Use dayjs instead of moment for better date handling ([c2b9d12](https://github.com/hanzydev/Fileship/commit/c2b9d12))
- Smaller emoji picker ([54bc7c3](https://github.com/hanzydev/Fileship/commit/54bc7c3))
- Smaller stats page ([21ca206](https://github.com/hanzydev/Fileship/commit/21ca206))

### 🏡 Chore

- Update dependencies to latest versions ([7b77d7e](https://github.com/hanzydev/Fileship/commit/7b77d7e))
- Remove execa ([5ee5368](https://github.com/hanzydev/Fileship/commit/5ee5368))
- Lint fix ([948128f](https://github.com/hanzydev/Fileship/commit/948128f))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.7

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.6...v1.2.7)

### 🩹 Fixes

- Color issue when hovering over version info ([d5b844d](https://github.com/hanzydev/Fileship/commit/d5b844d))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.6

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.5...v1.2.6)

### 💅 Refactors

- Prevent foldered files from appearing on the main page ([f7682d5](https://github.com/hanzydev/Fileship/commit/f7682d5))

### 🏡 Chore

- **ImpersonatingUser:** Center align content ([669aa2f](https://github.com/hanzydev/Fileship/commit/669aa2f))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.5

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.4...v1.2.5)

### 🩹 Fixes

- Fix issue where files disappear when a folder is deleted ([53aabb3](https://github.com/hanzydev/Fileship/commit/53aabb3))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.4

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.3...v1.2.4)

### 🩹 Fixes

- Fix folders not shown in the context menu of files ([09cc627](https://github.com/hanzydev/Fileship/commit/09cc627))
- Handle key events only when the color picker is open ([e00fe60](https://github.com/hanzydev/Fileship/commit/e00fe60))

### 🏡 Chore

- Fix repo name in build banner ([7ba6ebc](https://github.com/hanzydev/Fileship/commit/7ba6ebc))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.3

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.2...v1.2.3)

### 🩹 Fixes

- Fix folders not shown in the context menu of files ([09cc627](https://github.com/hanzydev/Fileship/commit/09cc627))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.2

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.1...v1.2.2)

### 🩹 Fixes

- Cannot read properties of null (reading 'insertBefore') error ([cca4725](https://github.com/hanzydev/Fileship/commit/cca4725))

### 🎨 Styles

- Fix text styles ([57ff137](https://github.com/hanzydev/Fileship/commit/57ff137))
- Fix border radius of version info button ([8e46f06](https://github.com/hanzydev/Fileship/commit/8e46f06))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.1

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.2.0...v1.2.1)

### 🏡 Chore

- Ignore CHANGELOG.md in prettier formatting ([6c3d3b7](https://github.com/hanzydev/Fileship/commit/6c3d3b7))

### 🎨 Styles

- Fix font family on charts ([f3effbd](https://github.com/hanzydev/Fileship/commit/f3effbd))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.2.0

[compare changes](https://github.com/hanzydev/Fileship/compare/v1.1.0...v1.2.0)

### 🚀 Enhancements

- **pkg:** Add release script ([3a2cefc](https://github.com/hanzydev/Fileship/commit/3a2cefc))
- **lint:** Better check script ([0bfccfb](https://github.com/hanzydev/Fileship/commit/0bfccfb))
- **UploadingFiles:** Show 'Starting' if progress is 0 ([f99c089](https://github.com/hanzydev/Fileship/commit/f99c089))
- Better loading indicator ([ac68a48](https://github.com/hanzydev/Fileship/commit/ac68a48))
- Add github link ([6c14800](https://github.com/hanzydev/Fileship/commit/6c14800))
- Add version info ([678436d](https://github.com/hanzydev/Fileship/commit/678436d))

### 🩹 Fixes

- Fix the backup that appears when it has not yet been created ([88b3592](https://github.com/hanzydev/Fileship/commit/88b3592))
- **pkg:** Fix docker compose command ([07ee2ed](https://github.com/hanzydev/Fileship/commit/07ee2ed))
- Fix outlined button ([4aeafaf](https://github.com/hanzydev/Fileship/commit/4aeafaf))
- Fix build banner ([9ec6dcf](https://github.com/hanzydev/Fileship/commit/9ec6dcf))

### 🎨 Styles

- Lint fix ([a799b6b](https://github.com/hanzydev/Fileship/commit/a799b6b))
- Change font to Quicksand ([172cdb5](https://github.com/hanzydev/Fileship/commit/172cdb5))

### ❤️ Contributors

- Hanzydev <contact@hanzy.dev>

## v1.1.0

[compare changes](https://github.com/hanzydev/Fileship/compare/1.0.0...v1.1.0)

### 🚀 Enhancements

-   CreateLog util & bug fixes ([4428f44](https://github.com/hanzydev/Fileship/commit/4428f44))
-   Add default limits for default user ([450f2e1](https://github.com/hanzydev/Fileship/commit/450f2e1))

### 🩹 Fixes

-   Delete user uploads when their account is deleted ([391d00b](https://github.com/hanzydev/Fileship/commit/391d00b))
-   **ImpersonateUser:** Fix paddings ([9300e8a](https://github.com/hanzydev/Fileship/commit/9300e8a))

### 🎨 Styles

-   Lint fix ([a95b8c0](https://github.com/hanzydev/Fileship/commit/a95b8c0))

### ❤️ Contributors

-   Hanzydev <contact@hanzy.dev>
