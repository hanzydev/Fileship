# PLAN.md

What is left before Fileship v6 ships, and the known gaps. Background and traps are in [MEMORY.md](MEMORY.md), architecture in [AGENTS.md](AGENTS.md). Tick items off (or delete them) in the commit that finishes them, and add new gaps as you find them.

## State of `v6-nextgen`

Done and in daily use:

- Dashboard rebuilt on shadcn-vue and the shared `packages/ui` layer, with the trash, storages (local, S3, SFTP, FTP), short links, inbox, backups, passkeys, TOTP and server-side sessions.
- AI analysis on a worker thread (`packages/ai`): image and text embeddings, captions, OCR, sensitive-content detection, and smart search with calibrated cutoffs; fulltext search works for every script.
- `POST /api/ai/ocr` for text capture.
- Telemetry panel (`apps/telemetry`) and the anonymous report schema (`packages/telemetry`).
- Fileship for Windows (`apps/desktop`): ShareX-style capture across monitors with annotations, OCR, screen and GIF recording, uploads with history, the File Explorer drive, auto-updates from `hanzydev/fileship-desktop`.

## Open gaps

Desktop:

- [ ] Check the region overlay on a mixed-DPI setup (for example 150% + 100%). The toolbar zoom and the overlay placement fallback have only been tested on two 100% monitors.
- [ ] Touch drags on the overlay are treated as clicks (the root has no `touch-action: none`).
- [ ] Recording border: when the monitors have different heights and the area runs past the shorter one, the edge inside the area on that monitor has no border line.
- [ ] Check a recording that spans two monitors end to end (border windows on both, gdigrab output size).
- [ ] Ask testers for the real-mouse feel of the new overlay; automated tests used CDP input only.

Dashboard and infrastructure:

- [ ] Build the Docker image (`docker build -f apps/dashboard/Dockerfile .`) and confirm the AI worker starts inside it (`.output/server/worker.mjs` and its traced dependencies). Only the local `.output` has been checked.
- [ ] Review the Dependabot alerts on `main` and the dependency updates before the release.
- [ ] Decide and document the upgrade path for 5.x instances (database migrations, data folder, sessions are not migrated because v6 drops backward compatibility).

## v6 release checklist

1. All open gaps above are fixed or accepted, and `pnpm check` passes.
2. Write the 6.0.0 section of `CHANGELOG.md`, listing every breaking change (the `!` commits).
3. Merge `v6-nextgen` into `main` with a pull request.
4. Publish the GitHub release `v6.0.0` in the Fileship repo; this builds and pushes the Docker images.
5. Set `apps/desktop/package.json` to `1.0.0`, push, and push the tag `desktop-v1.0.0`. Check that the release in `fileship-desktop` has the installer, its blockmap and `latest.yml`.
6. Install the desktop release on a clean Windows machine and run through login, capture, upload, OCR, recording and the drive.
