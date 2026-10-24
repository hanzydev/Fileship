# PLAN.md

What is left after Fileship 6.0.0 shipped, and the known gaps. Background and traps are in [MEMORY.md](MEMORY.md), architecture in [AGENTS.md](AGENTS.md). Tick items off (or delete them) in the commit that finishes them, and add new gaps as you find them.

## State

Shipped in 6.0.0 (and the Windows app 1.0.0):

- Dashboard rebuilt on shadcn-vue and the shared `packages/ui` layer, with the trash, storages (local, S3, SFTP, FTP), short links, inbox, backups, passkeys, TOTP and server-side sessions.
- AI analysis on a worker thread (`packages/ai`): image and text embeddings, captions, OCR, sensitive-content detection, and smart search with calibrated cutoffs; fulltext search works for every script.
- English, Turkish, German, French, Italian, Azerbaijani, Japanese, Korean and Simplified Chinese for the dashboard (UI and API errors) and the Windows app.
- Telemetry panel (`apps/telemetry`) and the anonymous report schema (`packages/telemetry`).
- Fileship for Windows (`apps/desktop`): ShareX-style capture across monitors with annotations, OCR, screen and GIF recording, uploads with history, Fileship Harbor in File Explorer, auto-updates from `hanzydev/fileship-desktop`.
- The upgrade from 5.4.4 was tested in Docker: migrations, files, folders, notes and passwords are kept; the steps are in the 6.0.0 section of `CHANGELOG.md`.

## Open gaps

Desktop:

- [ ] Check the region overlay on a mixed-DPI setup (for example 150% + 100%). The toolbar zoom and the overlay placement fallback have only been tested on two 100% monitors.
- [ ] Touch drags on the overlay are treated as clicks (the root has no `touch-action: none`).
- [ ] Recording border: when the monitors have different heights and the area runs past the shorter one, the edge inside the area on that monitor has no border line.
- [ ] Check a recording that spans two monitors end to end (border windows on both, gdigrab output size).
- [ ] Ask testers for the real-mouse feel of the new overlay; automated tests used CDP input only.
- [ ] Install 1.0.0 on a clean Windows machine and run through login, capture, upload, OCR, recording and Harbor.
- [ ] Check Harbor in a real Explorer (only scripted COM and registry tests ran): thumbnails of online-only images and videos, with the app running and closed (`-Embedding` start), the state columns, Share, the Windows 11 status flyout, the context menu verbs and whether Explorer needs a restart after the first start.
- [ ] Check the Explorer context menu "Upload to Fileship" with several selected files in a packaged build (one upload batch).
- [ ] Check the new Harbor flyout in a real Explorer: the quota bar (red from 90%, ∞ label and empty bar when unlimited), the more info message and its button, the sync status line, the five icon buttons, the paused state and the offline (`-Embedding`) view.
- [ ] Check in a real Explorer: the Views column (custom state without an icon), the Notes and Links folders, editing a note in a few editors, and the folder link verbs in a packaged build.
- [ ] Check the tray's Fileship Harbor submenu (state, storage line, pause and resume).
- [ ] Check the tray: a single click opens the region capture after the double-click time, a double click only opens the window.
- [ ] Check the 1.0.1 → next upgrade keeps the Harbor folder (`drive` / `driveRoot` store migration).
- [ ] Check nested folders in Harbor with a real Explorer (only the layout logic and the folder list were tested): folders several levels deep, creating, renaming and moving folders and files between levels, copying a folder tree in, deleting a tree, and moving into Notes or Links being refused.

Browser extension:

- [ ] Run it in Firefox: the upload tab, copying links from the background page, notifications, the context menus, `captureVisibleTab`, the area picker, the permission prompts and long uploads on the event page. Firefox was not installed on the test machine, so only `web-ext lint` ran on the Firefox build; every flow was tested in headless Chrome, against a mock server and against a local 6.1.0 dashboard.
- [ ] Check the host permission prompt from the context menu for an image on another site, in Chrome and Firefox (automated tests cannot click it, so they ran with the permission already granted).
- [ ] Check the file picker in the Chrome toolbar popup on macOS and Linux (Windows behaviour is assumed, not tested; the popup was only tested as a page).
- [ ] Publish to the Chrome Web Store, Edge Add-ons and addons.mozilla.org (store listings, screenshots, privacy answers that match the declared data collection).

Dashboard and infrastructure:

- [ ] Check the emails in real clients (Gmail, Outlook, Apple Mail, a dark mode client); they were only read as raw HTML and text from a local SMTP catcher.
- [ ] Send through a real SMTP server with STARTTLS and TLS; tests used a plaintext local catcher with AUTH PLAIN and LOGIN.

- [ ] The Docker image is 5.6 GB because the runner copies the whole workspace `node_modules`. Deploy only the dashboard's production dependencies.
- [ ] In the Docker image the AI worker starts and loads its dependencies, but downloading and running the models inside a container has not been tested.
- [ ] Review the Dependabot alerts and the open dependency updates.
- [ ] `/api/healthz` of the 5.4.4 image reports 5.4.3; check that future images report their own version.
- [ ] Team folders: editors cannot move files or folders yet (only the owner can), files do not record who uploaded them, and shared folders do not appear in Harbor.
- [ ] Scheduled backups were tested on the data directory with small accounts, with two production instances racing for the same schedules. Check a run on an S3 or SFTP active storage and with a large account.
- [ ] PDF thumbnails were tested in development and a production build on Windows. Check them in the Docker image (`pdf-worker.mjs` and `pdfium.wasm` in `.output`) and with encrypted and damaged PDFs.
- [ ] Duplicate detection was tested on the local storage only. Check the hashing reads (`hashFile`) on S3, SFTP and FTP, and a large backfill (`files:hash`) on an instance with many files.
- [ ] Check the share target on a real Android phone (installed from Chrome, over HTTPS): Fileship shows up for photos, videos, PDFs and other files (`accept: */*`), links from Chrome and text from other apps, large videos arrive, the logged-out flow keeps the items, and an app installed before the update picks up the new manifest. Only the service worker path was tested, with a synthetic multipart POST in a desktop browser.
