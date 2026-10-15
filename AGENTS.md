# AGENTS.md

Guidance for AI coding agents (and humans) working on Fileship, an open-source, self-hosted file and media management platform.

## Repository

pnpm + Turborepo monorepo (`pnpm@11`, workspaces `apps/*` and `packages/*`). pnpm uses its default isolated linker, so every workspace declares the packages it imports.

| Path                     | What it is                                                                                                                              |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------------------------- |
| `apps/dashboard`         | Nuxt 4 app: the UI and the Nitro API. Prisma/Postgres, socket.io, Orama search, local AI queue.                                         |
| `apps/updater-agent`     | Small h3 service (`src/index.ts`) that performs blue/green updates by driving `docker compose`.                                         |
| `apps/desktop`           | Fileship for Windows: an Electron app that captures, records and uploads to Fileship 6.0+, with a Nuxt renderer on the ui layer.        |
| `apps/telemetry`         | Nuxt 4 telemetry panel: receives the anonymous reports and shows the statistics. Admin only. Not part of the user-facing stack.         |
| `packages/ui`            | `@fileship/ui`, the shared Nuxt layer: shadcn-vue components, theme tokens, generic app components and the shared Nuxt modules.         |
| `packages/telemetry`     | `@fileship/telemetry`, the report schema (zod) and the client that collects runtime info and sends reports.                             |
| `packages/ai`            | `@fileship/ai`, the AI models (SigLIP, Florence-2, text embeddings, PaddleOCR, privacy filter) running on a worker thread.              |
| `packages/eslint-config` | `@fileship/eslint-config`: `.` for plain TypeScript packages, `./nuxt` for the rules the Nuxt apps and the ui layer pass to `withNuxt`. |

## Commands

Run from the repository root unless noted.

- `pnpm install`: install everything (pnpm only, enforced by `preinstall`).
- `pnpm dev`: run the dashboard in dev mode through Turbo. It generates the Prisma client and applies migrations first. The telemetry panel needs its own `.env` and runs with `pnpm --filter telemetry dev`.
- `pnpm build`: build every workspace.
- `pnpm lint`: ESLint in every workspace, then `prettier --write .`.
- `pnpm lint:types`: `nuxt typecheck` in the dashboard, the telemetry panel and the ui layer, plus `nuxt typecheck` and `tsc` for the main process in the desktop app. It must report zero errors.
- `pnpm check`: build + lint + lint:types, which is what CI runs (`.github/workflows/check.yml`, on Linux, where the desktop build skips the native addon and the drive helper).

Dashboard only (`pnpm --filter dashboard <script>`):

- `lint:code`: `eslint --fix .`
- `db:generate`, `db:deploy`, `db:migrate` (create-only), `db:prototype` (db push), `db:studio`, `db:reset`.

Desktop app only (`pnpm --filter desktop <script>`): `dev` (Nuxt on port 3020 plus Electron, restarts on main process changes) and `package` (builds `release/Fileship-Setup-<version>.exe` locally).

Telemetry panel only (`pnpm --filter telemetry <script>`): `dev` (port 3002), `db:generate`, `db:deploy`, `db:migrate` (create-only) and `db:studio`.

Turborepo behaviour can differ between versions. Before changing `turbo.json`, read the docs bundled with the installed package (`node_modules/turbo/docs`).

## Environment

- `apps/dashboard/.env.example` lists every dashboard variable. Only `DATABASE_URL` is required. Optional groups: storage (`NUXT_DATA_DIRECTORY`), site (`NUXT_PUBLIC_SITE_NAME`, `NUXT_PUBLIC_RETURN_HTTPS`), updater (`NUXT_UPDATER_URL`, `NUXT_UPDATER_SECRET`), trash (`NUXT_TRASH_RETENTION_DAYS`), logs (`NUXT_LOG_LEVEL`), crons (`DELETE_EXPIRED_CRON`, `GENERATE_STATS_CRON`), uploads (`NUXT_PUBLIC_FILE_CHUNK_SIZE`, `NUXT_REMOVE_EXIF_DATA`, `NUXT_AVATAR_MAX_SIZE`), Turnstile, PWA (`ENABLE_PWA`), telemetry and the `NUXT_AI_*` / `NUXT_PUBLIC_AI_ENABLED` group.
- Self-registration is off by default. `NUXT_PUBLIC_REGISTRATION_ENABLED=true` enables `/register` and `POST /api/auth/register`; `NUXT_REGISTRATION_PERMISSIONS` (only `UploadFiles`, `TakeNotes` and `ShortenLinks` are honoured) and `NUXT_REGISTRATION_USABLE_SPACE` (MB, `-1` unlimited) set what new accounts get.
- `NUXT_UPDATER_URL` and `NUXT_UPDATER_SECRET` point the dashboard at the updater-agent. `docker-compose.yml` sets both.
- The updater-agent reads `PORT`, `SECRET_TOKEN`, `DEV_MODE`, `AUTOUPDATER_ENABLED`, `AUTOUPDATER_CRON`, `COMPOSE_FILE` and `FILESHIP_URL`.
- `NUXT_TELEMETRY_URL` sends the dashboard's reports to another panel. Development builds only send reports when it is set.
- The telemetry panel (`apps/telemetry/.env.example`) reads `DATABASE_URL`, `NUXT_ADMIN_PASSWORD` (the panel stays locked while it is empty) and `NUXT_INSTANCE_RETENTION_DAYS`.
- Instance settings (site name, link protocol, uploads, trash retention, registration, Turnstile, AI, log level, telemetry, active storage) are stored in the `Setting` table and edited by super admins at `/dashboard/instance`. The env/runtimeConfig values are only their defaults. Server code reads them with `getSettings()` (or `getSettingsSync()` in synchronous code) from `server/utils/settings.ts`; the client reads the public subset with `usePublicSettings()`, kept in sync over the `settings:update` socket event. The schema lives in `shared/utils/settings.ts`.
- Server code reads the remaining configuration through `useRuntimeConfig()`, never `process.env`. Runtime overrides therefore use the `NUXT_` prefix. The exceptions are `DATABASE_URL` (shared with the Prisma CLI) and the build-time options in `nuxt.config.ts` (`ENABLE_PWA`, `DELETE_EXPIRED_CRON`, `GENERATE_STATS_CRON`), which only take effect when the app is built.
- Never commit `.env` files, `data/` or anything under `.claude/` or `.impeccable/`.

## Deployment model

`docker-compose.yml` runs `postgres`, a Caddy `proxy` (`caddy.json`), `fileship-blue`, `fileship-green` (profile `green`) and `updater-agent`.

- Caddy uses the `first` selection policy with active health checks on `/api/healthz`, so traffic goes to the first healthy color.
- An update pulls the image for the idle color, starts it, waits for `ready: true`, and then stops the old color. If the new color exits, dies or never becomes healthy, the agent stops it again and the old color keeps serving.
- `/api/healthz` returns `{ status, version, ready, busy }`. `busy` is true while any backup restore is running; the agent waits for it (up to 10 minutes) before stopping the old color, and the auto-updater skips a run when busy.
- Both colors share the database and `data/`. Startup work must be idempotent and safe to run twice (for example, `resetBackupRestoreState` only clears restores older than 2 hours, and `storage:sync` skips a storage that cannot be listed or looks empty).
- `docker-compose.dev.yml` is the same stack built from local Dockerfiles, with development settings (no container health checks, the updater on port 3001 in dev mode, no auto-updates).

## Shared UI layer (`packages/ui`)

- A Nuxt layer shipped as source. The dashboard and the telemetry panel use it through `extends: ['@fileship/ui']`, and its `components`, `composables`, `utils`, `layouts` and `plugins` are auto-imported there.
- `app/components/ui`: shadcn-vue components (style `reka-luma`, see `packages/ui/components.json`). Run `shadcn-vue add` from `packages/ui`.
- Generic components: `AppHeader` (sidebar trigger, breadcrumbs, theme toggle), `PageHeader`, `TextBlock`, `EmptyState`, `Centered`, `StatCard`, `StatsDonut`, `TrendChart`, `DataTable`, `VirtualList`, `ListCardSkeleton`, `PasswordInput`, `ConfirmDialog` and `AppToaster` (the Sonner toaster every app renders: `rounded-3xl` toasts with `buttonVariants` action and cancel buttons; `tailwind.css` releases vue-sonner's unlayered radius and button styles with `revert-layer` so those classes apply). Composables: `useConfirm`, `useQueryState`, `useBreadcrumbs`, `useBreadcrumbLabels`. Utils: `cn`, `pluralize`, `clamp`, `getErrorMessage`.
- `app/error.vue` is the error page of every app. Its home button leads to the `home` app config (`{ path, label }`), which each app sets in its own `app/app.config.ts`.
- `app/styles/tailwind.css` holds the Fileship theme tokens (light and dark). Each app imports it from its own `app/styles/tailwind.css` (`@import '@fileship/ui/app/styles/tailwind.css';`). The tokens set `color-scheme`, and the layer's `color-scheme` plugin writes the matching `<meta name="color-scheme">` from the color-mode preference (on the server from the cookie), so a browser paints the right canvas before the stylesheets load. The dashboard adds its accent colors after it; the panel uses only the default colors.
- Inside the layer, `@/` and `~/` point to the app that extends it. Import layer files through `#layers/ui/...` instead, for example `#layers/ui/app/components/ui/sidebar`.
- The layer declares every module and package it uses (`@nuxt/fonts`, `@nuxtjs/color-mode`, `@vueuse/nuxt`, `vue-sonner/nuxt`, `nuxt-lucide-icons`, `@nuxt/eslint`, Tailwind, reka-ui, Unovis, ...). Apps add only what they import themselves.
- Put something here only when both apps use it or it is part of the design system. Anything that needs `useAuthUser`, the socket or Fileship APIs stays in the dashboard.

## Dashboard layout (`apps/dashboard`)

- `app/pages`: route groups `(auth)` (login, register), `(dashboard)/dashboard/*` (home, files, folders, notes, links, trash, logs, stats, users, instance, settings/\*) and `(view)` (public `view/[id]`, `folder/[id]`, `note/[id]`, `inbox/[id]`, `link/[code]`).
- `app/layouts`: `dashboard.vue` (sidebar shell, command palette, upload sheet and queue, global drop zone, paste upload). Public pages use the layer's `default.vue`.
- `app/middleware`: `userOnly`, `guestOnly`, `adminOnly`, `superAdminOnly`, `fileUploaderOnly`, `noteTakerOnly`, `linkShortenerOnly`.
- `app/components`: app components, flat. The shadcn-vue components come from the ui layer.
- `app/composables`: `useState` stores plus action composables (`useFileActions`, `useFolderActions`, `useNoteActions`).
- `app/types`: the client data types (`FileData`, `FolderData`, `NoteData`, `LinkData`, `UserData`, `LogData`, `SessionData`, `BackupData`), imported with `import type` from `~/types/<name>`.
- `app/utils`: client helpers (`socket.ts`, `uploadFile.ts`, ...).
- `server/api`: REST endpoints (`ai`, `auth`, `files`, `folders`, `inbox`, `links`, `logs`, `notes`, `settings`, `storages`, `trash`, `updater`, `users`, `stats`, `healthz`).
- `server/routes/u/[id]`: raw file serving. `server/routes/s/[code]`: short link redirects.
- `server/middleware`: `auth.ts` (the `token` session cookie or the same session token as `Authorization: Bearer`, plus the upload token on `POST /api/files` and `POST /api/links`) and `cors.ts`.
- `server/plugins`: `setup.ts`, `socketIO.ts`, `aiQueueRunner.ts`.
- `server/tasks`: `db:*`, `dir:*`, `search:*`, `ai:*`, `media:*`, `storage:*`, `telemetry:*`, scheduled in `nuxt.config.ts` or started from `setup.ts`.
- `server/storage`: storage drivers (`local`, `s3`, `sftp`, `ftp`) behind the `StorageDriver` interface in `types.ts`.
- `server/utils`: list/query helpers (`list.ts`), permissions, Prisma, Orama search, AI queue, sessions (`session.ts`), thumbnails and metadata (`media.ts`), storage (`storage.ts`, `upload.ts`, `backup.ts`, `fileData.ts`), `verifySession`, `createLog`, `errors.ts`, ...
- `shared/utils`: code used by both sides (constants, file categories, permissions, types, accents).
- `prisma/schema.prisma` and `prisma/migrations`. The client is generated into `generated/prisma` (gitignored).

## Key building blocks

Reuse these instead of writing new versions.

- `usePaginatedList(resource, endpoint, params, options)` (`app/composables/usePaginatedList.ts`): infinite list state (`items`, `total`, `summary`, `isLoading`, `isError`, `hasMore`, `loadMore`, `refresh`). Pages show an `EmptyState` with a retry (`refresh`) while `isError` is set; a failed background refresh keeps the items already shown. Socket events update every open list through `refreshLists`, `patchListItem`, `removeListItems` and `mapListItems`. `useListHandle` registers single items for detail pages.
- `DataTable.vue`: window-virtualized table. Columns support `sortable` and `descFirst`; it switches to a `#card` slot below 640px of container width and emits `reachEnd`.
- `VirtualList.vue`: window-virtualized responsive grid (`minItemWidth`, `gap`, `estimateSize`, `reachEnd`). With `contained` it virtualizes against its nearest scrolling ancestor instead (dropdowns, command lists, dialogs).
- `useQueryState(key, default)`: a ref bound to the URL query. It drops defaults and batches updates.
- `askConfirmation({ ..., action })` (`useConfirm.ts`): confirmation dialog that keeps a spinner until `action` resolves.
- `withVerification(action)` (`useVerification.ts`): retries an action after a password, TOTP or passkey prompt when the API asks for verification.
- `getErrorMessage(error)`: user-facing message from a fetch error.
- Storage: never build `data/uploads` or `data/backups` paths. Upload and backup bytes live on a storage (`File.storageId` / `Backup.storageId`, `null` is the data directory) under the keys `uploadKey(fileName)` (`uploads/<name>`) and `backupKey(backup)` (`backups/<userId>/<id>.tgz`). Use `getStorage(storageId)` for `read`, `put`, `move`, `remove` and `list`, and `withLocalFile(storageId, key, fn)` when a tool needs a real path (sharp, ffmpeg, AI, tar). `removeFileData(files)` removes a file's bytes, thumbnail and search entry.
- Chunked uploads go through `receiveUpload` (files, inbox) or `receiveChunk` (backups) in `server/utils/upload.ts`. Chunks are written straight to the active storage, except images that need EXIF removal or compression, which are assembled in `data/temp` first. Upload sessions live in `data/temp/<owner>/<uploadId>.upload.json` and `dir:clearTemp` aborts stale ones. Chunks may arrive in any order and in parallel: the session is read and written under a per-upload lock, the bytes are written outside it at `(part - 1) * chunkSize`, every chunk must match that layout, and the upload completes when all `totalChunks` parts have arrived. A storage driver returns per-part data from `writeChunk` (S3 ETags), which `completeUpload` receives. FTP appends and is marked `isSequential`, so `/api/settings/public` sends `uploadConcurrency: 1` for it and 4 otherwise.
- Trash: `File`, `Folder` and `Note` have `deletedAt`. Every query that lists, counts, serves or edits them must add `deletedAt: null` (use `userCountSelect` from `list.ts` for user counts); only the trash endpoints and `server/utils/trash.ts` (`deleteTrashed`, `delete*Forever`, `filesTrashedWith`) work on trashed rows. Storage quotas (`exceedsUsableSpace`) and the stats count trashed files, and the stats report the trashed size separately. User deletes go to the trash while `trashRetentionDays > 0`; expiry, max views, `storage:sync`, user deletion and backup restores delete immediately. `db:purgeTrash` runs hourly.
- File names: `File.fileName` is the unique URL slug (`/u/<fileName>`, ASCII only, unique across the instance). `File.name` is the display name: the original upload name with Unicode, only Windows-invalid characters stripped, not unique. Show and sort by `name` in the UI, use it for downloads (`Content-Disposition`), and keep links on `fileName`.
- Links: `Link` rows are served by `/s/[code]` through `findActiveLink` and `countLinkClick` (`server/utils/link.ts`). Owner clicks are not counted, a link is deleted at its click limit, and expired links are removed by `db:deleteExpired`.
- `storage:sync` runs on startup and removes files and backups whose bytes are missing, and indexes backup archives that have no row. It skips rows created in the last 15 minutes and archives of pending upload sessions, because the other color may still be finishing them. `storage:migrate` moves everything to the active storage.
- AI analysis: one job per file (`AnalyzeImage`, `AnalyzeVideo`, `AnalyzeText`, plus `DetectPII` when an override is cleared), queued with `enqueueFileAnalysis` and run by `runAIJob` (`server/utils/aiJobs.ts`) from the `aiQueueRunner` pool. A job downloads the file once, runs each step (image embedding, OCR, caption, text embedding, PII) and saves it right away, skipping steps whose data already exists, then sets `File.aiAnalyzedAt`. Queue guarantees (`server/utils/aiQueue.ts`): one active job per file and type (partial unique index), atomic claims with `FOR UPDATE SKIP LOCKED`, a lease renewed while the job runs, retry after the lease on errors, `Failed` after `AI_MAX_ATTEMPTS`, and finished jobs are deleted. `ai:syncJobs` queues files whose analysis is missing or was made with other models (`embeddingModel` / `textEmbeddingModel`); failed files wait for **Retry failed**, and `ai:cleanup` deletes failed jobs after 30 days so those files are queued again. `resetAIData()` (`POST /api/settings/ai-reset`) clears every file's AI data except owner overrides, deletes all jobs, rebuilds the search index and runs the sync.
- AI models and presets: `server/utils/aiPresets.ts` defines `low`, `medium` and `high` (instance setting `aiPreset`), each with its models, search cutoffs, OCR tier, keep-alive and video frames. Inference runs synchronously inside onnxruntime, so every model lives in `packages/ai` and runs on a worker thread; the main thread only talks to it through the `ai` facade in `server/utils/ai.ts` (`createAIClient`), which sends the active preset, `aiInferenceThreads` and `data/models` with every call. Pass file paths or buffers, never decoded images. A call that is running when the worker dies is rejected (the job retries) and the next call starts a new worker; the worker stops when Nitro closes. Nitro inlines workspace packages, so the `compiled` hook in `nuxt.config.ts` copies the worker to `.output/server/worker.mjs` and `traceInclude` brings its dependencies. Never import transformers, onnxruntime or ppu-paddle-ocr in the dashboard. Inside the worker, models load through `useAIModel` (`packages/ai/src/models.ts`), which shares one instance, retries after a failed load, unloads after the keep-alive and uses `getSessionOptions()` (ONNX memory arena off). PII rules (`findPii`) stay on the main thread; the worker only returns the privacy filter entities. OCR is `ppu-paddle-ocr` (`packages/ai/src/ocr.ts`): PP-OCRv6 detects text once and its boxes are recognised, then PP-OCRv5 script models run only when text regions were left unread. Choose models by measuring them (multilingual retrieval R@1, results returned for unrelated queries, OCR error rate, peak RSS within the preset's RAM budget), not by benchmarks alone; a new embedding model needs a new `key`, and its search cutoff must be calibrated.
- Sensitive content: `ai.detectPII` runs the OpenAI Privacy Filter over the OCR text or file text (and uses the image caption to recognize photos of cards and ID documents), and `findPii` (`server/utils/pii.ts`) turns its spans into reasons and adds the structural validators (Luhn with card network prefixes, IBAN mod 97, ICAO MRZ check digits). Keep it free of country-specific rules. Reason codes and their labels are in `shared/utils/pii.ts`. `PERSON_NAME` and `PRIVATE_DATE` only count next to another reason or together. `File.piiOverride` is the owner's choice (`PATCH /api/files/[id]` with `sensitive`); `DetectPII` never changes a file that has one, and `null` hands it back to the model.
- Search index: write file entries with `upsertFileSearchEntry(fileId)` (`server/utils/search.ts`), which re-reads the row and serializes writes, never with Orama `insert`/`update` directly. Orama only holds text; vectors live in `server/utils/vectorIndex.ts` and only those made by the active preset's models are loaded (`reloadFileVectors`). `search:refresh` indexes files analyzed by the other blue/green color. `searchFileIds` runs fulltext only for visitors, when AI is off for the instance or the user, or in fulltext mode; smart mode fuses the image, text and whole-word fulltext lists, each cut by its model's calibrated floor and ratio, and falls back to fulltext while a query model is still loading.
- `app/utils/socket.ts`: socket.io client that keeps user, theme, embed, domains, passkeys and lists in sync.

## Telemetry

- The report schema is `reportSchema` in `packages/telemetry/src/schema.ts`. The dashboard (`server/tasks/telemetry/collect.ts`) and the panel both validate against it, so change it in one place and keep older reports readable.
- Reports are anonymous: counts, sizes, on/off flags and version strings only. Never add names, file names, captions, URLs, domains, hosts, IP addresses or setting values such as keys. The instance id is the random `data/.fileship_project_id`.
- The dashboard collects nothing while telemetry is off. It sends one report five minutes after startup and then every six hours (`POST /api/v2/report`). Instances older than 6.0 still call `POST /api/event`, which the panel keeps accepting.

## Telemetry panel (`apps/telemetry`)

- Same structure and conventions as the dashboard, on the ui layer, with the default Fileship colors and a light/dark toggle.
- `Report` keeps one row per instance and UTC day (`@@unique([instanceId, day])`). Ingestion (`saveReport` in `server/utils/ingest.ts`) upserts that row; legacy events are merged into it and carry the instance's last known values forward. Ingestion is rate limited per instance and per client (at most 5 new instances per client a day) and never stores IP addresses.
- An instance is active on a day when it reported in the `ACTIVE_DAYS` (7) days up to and including it; a day's totals add up the latest report of every active instance (`computeDay` in `server/utils/rollup.ts`). `DailyStat` stores those totals. `stats:rollup` (hourly) fills missing days and recomputes only today and yesterday, so pages read ready rows and past days never change.
- `db:prune` (daily and on startup) runs the rollup first, then deletes instances silent for `NUXT_INSTANCE_RETENTION_DAYS` with their reports, reports older than 13 months and expired sessions. `DailyStat` is never pruned.
- Access is a single admin password (`NUXT_ADMIN_PASSWORD`). Sessions are random tokens stored as SHA-256 hashes, sent in the httpOnly `telemetryToken` cookie and expiring after 30 days. Failed logins are rate limited per IP and across all addresses, and cookie-authenticated writes reject cross-site requests like the dashboard.

## Desktop app (`apps/desktop`)

- **Processes:**
    - `electron/` is the main process. tsdown bundles it, with every dependency, into `dist-electron/main.cjs` and `preload.cjs`, so the package ships no `node_modules`. Keep runtime packages in `devDependencies`.
    - Native Windows work goes through `fileship-native.node`, a C++ N-API addon in `native/core/` (loaded by `electron/native.ts`): GDI screen capture on worker threads (one per display), window and DXGI output enumeration, focusing windows, and PNG/JPEG encoding with WIC off the main thread. `scripts/native.mjs` builds it with MSVC (shared setup in `scripts/msvc.mjs`) into `build/native`, which ships as `resources/native`; it links `node.exe` through a delay-load hook, so one build works in every Electron version. Electron forbids external buffers, so results go into buffers allocated in JS.
    - `app/` is a Nuxt SPA on the ui layer, served from the `app://fileship` protocol (`electron/protocol.ts`).
- **IPC:** the renderer only talks to the main process through the typed bridge in `shared/ipc.ts` (`window.fileship`, `desktop(channel, ...args)`). Add every channel to `IpcInvoke`/`IpcEvents` and the channel lists. Handlers in `electron/ipc.ts` reject senders that are not app pages.
- **Login:** the instance's own `/login` runs in a temporary partition. Its httpOnly `token` cookie becomes the app's session, kept DPAPI encrypted (`safeStorage`) and sent as `Authorization: Bearer`. The user agent carries `FileshipDesktop/<version>`, so the session shows as "Fileship for Windows". The app requires Fileship `MIN_FILESHIP_VERSION` (6.0.0) from `/api/healthz`.
- **Dashboard:** file management is not rebuilt in the app. The Dashboard window loads the instance itself, in the `persist:dashboard` partition with the same session cookie.
- **Updates:** electron-updater uses the GitHub provider with the public `hanzydev/fileship-desktop` releases repo. `.github/workflows/desktop-release.yml` builds and publishes on `desktop-v*` tags (`DESKTOP_RELEASE_TOKEN` secret). Updates download in the background and install on restart or quit, never while `setBusyCheck` reports work in progress.
- **Capture:** global hotkeys (`electron/hotkeys.ts`, editable, ShareX defaults) and the tray run actions from `electron/actions.ts`. Region capture uses one overlay window per display (`electron/overlay.ts`, page `app/pages/overlay.vue`), which gets a frozen BGRA snapshot and the window rects, and shows only after the page has painted. Overlays are created when a capture starts (the page reports `overlay:loaded`) and destroyed two minutes after the last one; on close the page shrinks its canvases to 0×0 and the annotator unmounts its Konva stage, so nothing stays allocated between captures. Once the pixels are taken, `playShutter` (`electron/capture/index.ts`) plays `resources/shutter.wav` through the native addon's `playSound` (PlaySound from memory, since the file sits in the asar) unless `capture.playSound` is off. `finishUpload` plays `resources/uploaded.wav` the same way unless `afterUpload.playSound` is off (drive uploads stay silent); `scripts/sounds.mjs` derives it from the shutter sound. Results go through `handleImage`/`handleFile` (`electron/tasks.ts`) for the after-capture and after-upload tasks, uploads are chunked through `electron/upload.ts`, which sends up to the server's `uploadConcurrency` chunks at once and reports progress to the app, the tray tooltip and the taskbar, and every item lands in the SQLite history (`electron/history.ts`, `node:sqlite`). Text capture (`captureText`, `electron/ocr.ts`) opens the overlay with the `ocr` purpose, which has no annotations and a Copy text button, sends the region as PNG to `POST /api/ai/ocr` and copies the text; the endpoint runs the instance's OCR model without storing anything, needs AI on for the instance and the user, and is rate limited per user.
- **Annotations:** `useAnnotator` (Konva) holds the shapes as plain data with undo/redo and renders them on a stage above a source canvas; blur and pixelate crop that source. `AnnotationToolbar` is shared by the region overlay, where annotations are drawn while selecting, and the editor window (`electron/editor.ts`, `app/pages/editor.vue`), which opens after a capture when `afterCapture.annotate` is on and for saved screenshots in the history. Both export with `renderRegion` and send RGBA back to the main process.
- **Recording:** `electron/recording/`. ffmpeg is the pinned gyan.dev build that `ffmpeg.ts` downloads to `userData/ffmpeg` (SHA-256 checked) or a custom path; encoders are probed once per binary. Video comes from `ddagrab` on the DXGI adapter and output that hold the area (`listOutputs` in `capture/native.ts`), or `gdigrab` for areas that span monitors. Audio is captured in the recorder toolbar page (`app/pages/recorder.vue`: microphone through `getUserMedia`, system audio through `getDisplayMedia` loopback), mixed or split into tracks in WebAudio and streamed as f32le PCM through one named pipe per track. Each pipe is primed with silence, and audio starts at the sample of the first video frame (`showinfo`). After `q` the pipes are closed, or ffmpeg waits on them forever. Pausing ends the segment; segments are joined with the concat demuxer, and GIFs are recorded losslessly and converted with a palette pass. The toolbar and border use content protection, so they never appear in recordings.
- **Drive:** the user's files and folders in File Explorer through the Windows Cloud Files API.
    - `native/drive/` is a C++20 / C++/WinRT helper (`fileship-drive.exe`) built with MSVC by `scripts/drive.mjs` into `build/drive`; it needs Visual Studio Build Tools with the x64 C++ toolset and ships as `resources/drive`.
    - It registers the sync root (`Fileship!<SID>!<account key>`, always-full population), creates placeholders with identities `f:<fileId>` / `d:<folderId>`, hydrates them itself with WinHTTP range requests to `/u/<id>`, asks before renames and moves (including moves to the Recycle Bin, which leave the sync root), reports completed deletes (`NOTIFY_DELETE_COMPLETION`, skipped when the same placeholder is still there, because the shell probes with a delete before recycling), and watches for new and edited files. Before a drive upload the file is turned into a not-in-sync placeholder (`prepare`, new files get a temporary `p:` identity), its progress goes to Explorer's Status column with `CfReportProviderProgress` (`progress`), and `adopt` or `update` sets the real identity and marks it in sync. An edit is a placeholder whose last write time differs from its creation time.
    - `electron/drive/` owns the Fileship side: it loads folders and files, maps names to safe, unique Windows paths (`paths.ts`), applies socket events, and turns Explorer actions into API calls (Recycle Bin → trash, a completed delete such as Shift + Delete → trash and then `POST /api/trash/delete`, rename and move → `PATCH`, new file → upload, edited file → upload as a new file and trash the old one). Fileship folders are flat, so nested folders are refused.
    - The drive only uses an empty folder: choosing a non-empty one is refused, and a start in a non-empty folder that holds no Fileship placeholders is undone (`driveRoot` in the store remembers the folder the drive owns; logging out forgets it).
    - Logging out or turning the drive off removes only placeholders that are in sync (scanned first), so edits that were not uploaded, pending uploads and ordinary files stay on disk; then it unregisters the sync root. The uninstaller runs `--unregister-all` except during updates.
- **Clipboard:** Electron 44's clipboard is async. Use the helpers in `electron/clipboard.ts`.
- **Windows:** the main window exists only while it is open: `showMainWindow` creates it, closing destroys it, and the app keeps running in the tray. A `--hidden` start creates no window. Fileship starts with Windows (`--hidden`) and keeps a single instance.

## Conventions

- Code style: no comments, `handle*` names for event handlers, `<template>` before `<script setup>`, `definePageMeta` last, non-null `!` and `as any` are fine where the type is known.
- Prettier: 4 spaces, single quotes, semicolons, trailing commas, width 100. Format only the files you changed.
- Create a component only when it is used in more than one place.
- Errors: `createError({ status, statusText?, message, data? })`. `statusCode` and `statusMessage` are deprecated.
- Long lists use virtual scroll with `reachEnd`, never pagination controls.
- Commits: conventional commits (`feat`, `fix(ui)`, `fix(api)`, `chore`, `docs`, ...), one topic per commit.

## UI rules

- Use shadcn-vue components. Read `https://www.shadcn-vue.com/docs/components/<name>` before using one and follow its documented composition.
- Never restyle `packages/ui/app/components/ui`. Small upstream logic or attribute fixes are acceptable; colors, radii and spacing are not changed there. Adjust at the usage site instead.
- Add new shadcn components from the `reka-luma` registry in `packages/ui` and keep them unmodified. Prettier skips that folder.
- No `cursor-pointer`.
- Icons come from lucide through `nuxt-lucide-icons` (`<IName />`).
- Use theme tokens (`bg-muted`, `text-muted-foreground`, `var(--primary)`, ...) instead of raw colors, so accents and dark mode keep working.
- Both apps share one design language: build the panel's pages from the same components and layout patterns as the dashboard.

## Security notes

- `/u/[id]` serves user uploads with `X-Content-Type-Options: nosniff` and `Content-Security-Policy: sandbox`; HTML, SVG, XML and JavaScript are served as attachments. `/u/[id]`, thumbnails and `GET /api/files/[id]` look a file up by id first, then by `fileName` (`fileLookups` in `server/utils/fileAccess.ts`). Visitors get 404 for trashed, expired and sensitive files (`isHiddenFromVisitors`) and never receive internal fields (`visitorFileFields`). A counted view is claimed once per IP, user agent and file per 10 minutes (`claimView`), and the view that reaches `maxViews` is still served; the file is deleted when that response closes.
- Orama indexes are shared; every search must filter by `authorId`.
- Public folders list password protected files to visitors only as `{ id, fileName, mimeType, size, createdAt, isLocked: true }` (their `name` is the `fileName`), matched by `fileName` only; their bytes need the password (`GET /api/files/[id]` is rate limited per IP and file). Sensitive files (`piiDetected`) are never shown to visitors, and folder archives leave out locked and sensitive files. Visitor queries use `visitorFileWhere(owner)`, which also hides images and text files that are not analyzed yet while AI is on.
- Backup restores accept an explicit allow-list of fields per model and always force the owner to the current user.
- Upload ids, file names and paths are validated before touching the filesystem.
- Sensitive account actions go through `verifySession` on the server and `withVerification` on the client.
- Sessions are random tokens stored as SHA-256 hashes (`Session.tokenHash`). Browsers send them only in the httpOnly `token` cookie and the web client never reads or writes auth cookies. Native clients such as the desktop app send the same token as `Authorization: Bearer`, which works on every route and on the socket, and skips the cross-site check because browsers never send it on their own. Sessions have no expiry and end only when they are revoked. Impersonation runs on the server (`/api/auth/impersonate`), with the admin's token kept in the httpOnly `adminToken` cookie.
- Storage configs are stored AES-GCM encrypted with the same `<data>/secret.key`. The API never returns secret fields; it sends `<field>: true/false` flags, and an empty secret on update keeps the stored value.
- Upload tokens (`fs_…`) are accepted only on `POST /api/files` and `POST /api/links`, through the `Authorization` header. They are stored hashed for lookup and AES-GCM encrypted for display, with the key in `<data>/secret.key`.
- Client IPs come from `getClientIp(event)` (`server/utils/clientIp.ts`), never `getRequestIP` with `xForwardedFor`: it trusts `X-Fileship-Client-IP` (set by Caddy from `{client_ip}` with `trusted_proxies_strict`) or the last `X-Forwarded-For` entry only when the peer is a private address or an internal hop, and otherwise uses the socket address.
- Login, passkey login, registration and step-up verification are rate limited (`rateLimit`). TOTP codes cannot be reused (`verifyTotpToken`). Cookie-authenticated writes are rejected when the browser marks them as cross-site (`Sec-Fetch-Site` other than `same-origin` or `none`). The check does not depend on the `Host` header, so it works behind any reverse proxy.
- Short links only accept `http` and `https` targets. Password protected links redirect to `/link/[code]`, and `POST /api/links/unlock` is rate limited.
- Markdown is rendered without raw HTML, and links are limited to `http`, `https`, `mailto` and relative URLs.
