# PLAN.md

What is left after Fileship 6.0.0 shipped, and the known gaps. Background and traps are in [MEMORY.md](MEMORY.md), architecture in [AGENTS.md](AGENTS.md). Tick items off (or delete them) in the commit that finishes them, and add new gaps as you find them.

## State

Shipped in 6.0.0 (and the Windows app 1.0.0):

- Dashboard rebuilt on shadcn-vue and the shared `packages/ui` layer, with the trash, storages (local, S3, SFTP, FTP), short links, inbox, backups, passkeys, TOTP and server-side sessions.
- AI analysis on a worker thread (`packages/ai`): image and text embeddings, captions, OCR, sensitive-content detection, and smart search with calibrated cutoffs; fulltext search works for every script.
- English, Turkish, Simplified Chinese, German, French and Italian for the dashboard (UI and API errors) and the Windows app.
- Telemetry panel (`apps/telemetry`) and the anonymous report schema (`packages/telemetry`).
- Fileship for Windows (`apps/desktop`): ShareX-style capture across monitors with annotations, OCR, screen and GIF recording, uploads with history, the File Explorer drive, auto-updates from `hanzydev/fileship-desktop`.
- The upgrade from 5.4.4 was tested in Docker: migrations, files, folders, notes and passwords are kept; the steps are in the 6.0.0 section of `CHANGELOG.md`.

## Open gaps

Desktop:

- [ ] Check the region overlay on a mixed-DPI setup (for example 150% + 100%). The toolbar zoom and the overlay placement fallback have only been tested on two 100% monitors.
- [ ] Touch drags on the overlay are treated as clicks (the root has no `touch-action: none`).
- [ ] Recording border: when the monitors have different heights and the area runs past the shorter one, the edge inside the area on that monitor has no border line.
- [ ] Check a recording that spans two monitors end to end (border windows on both, gdigrab output size).
- [ ] Ask testers for the real-mouse feel of the new overlay; automated tests used CDP input only.
- [ ] Install 1.0.0 on a clean Windows machine and run through login, capture, upload, OCR, recording and the drive.

Dashboard and infrastructure:

- [ ] The Docker image is 5.6 GB because the runner copies the whole workspace `node_modules`. Deploy only the dashboard's production dependencies.
- [ ] In the Docker image the AI worker starts and loads its dependencies, but downloading and running the models inside a container has not been tested.
- [ ] Review the Dependabot alerts and the open dependency updates.
- [ ] `/api/healthz` of the 5.4.4 image reports 5.4.3; check that future images report their own version.
