# AGENTS.md

Guidance for AI coding agents (and humans) working on Fileship, an open-source, self-hosted file and media management platform.

## Repository

pnpm + Turborepo monorepo (`pnpm@11`, `node-linker=hoisted`, workspaces `apps/*` and `packages/*`).

| Path                     | What it is                                                                                                                           |
| ------------------------ | ------------------------------------------------------------------------------------------------------------------------------------ |
| `apps/dashboard`         | Nuxt 4 app: the UI and the Nitro API. Prisma/Postgres, socket.io, Orama search, local AI queue.                                      |
| `apps/updater-agent`     | Small h3 service (`src/index.ts`) that performs blue/green updates by driving `docker compose`.                                      |
| `packages/telemetry`     | `@fileship/telemetry`, anonymous usage telemetry.                                                                                    |
| `packages/eslint-config` | `@fileship/eslint-config`, shared flat config used by the updater-agent and telemetry. The dashboard has its own `eslint.config.js`. |

## Commands

Run from the repository root unless noted.

- `pnpm install`: install everything (pnpm only, enforced by `preinstall`).
- `pnpm dev`: run every app in dev mode through Turbo. The dashboard generates the Prisma client and applies migrations first.
- `pnpm build`: build every workspace.
- `pnpm lint`: ESLint in every workspace, then `prettier --write .`.
- `pnpm check`: build + lint, which is what CI runs (`.github/workflows/check.yml`).

Dashboard only (`pnpm --filter dashboard <script>`):

- `lint:types`: `nuxt typecheck`. It must report zero errors.
- `lint:code`: `eslint --fix .`
- `db:generate`, `db:deploy`, `db:migrate` (create-only), `db:prototype` (db push), `db:studio`, `db:reset`.

Turborepo behaviour can differ between versions. Before changing `turbo.json`, read the docs bundled with the installed package (`node_modules/turbo/docs`).

## Environment

- `apps/dashboard/.env.example` lists every dashboard variable. Only `DATABASE_URL` is required. Optional groups: storage (`NUXT_DATA_DIRECTORY`), crons (`DELETE_EXPIRED_CRON`, `GENERATE_STATS_CRON`), uploads (`NUXT_PUBLIC_FILE_CHUNK_SIZE`, `NUXT_REMOVE_EXIF_DATA`, `NUXT_AVATAR_MAX_SIZE`), Turnstile, PWA (`ENABLE_PWA`), telemetry and the `NUXT_AI_*` / `NUXT_PUBLIC_AI_ENABLED` group.
- Self-registration is off by default. `NUXT_PUBLIC_REGISTRATION_ENABLED=true` enables `/register` and `POST /api/auth/register`; `NUXT_REGISTRATION_PERMISSIONS` (only `UploadFiles`, `TakeNotes` and `ShortenLinks` are honoured) and `NUXT_REGISTRATION_USABLE_SPACE` (MB, `-1` unlimited) set what new accounts get.
- `NUXT_UPDATER_URL` and `NUXT_UPDATER_SECRET` point the dashboard at the updater-agent. `docker-compose.yml` sets both.
- The updater-agent reads `PORT`, `SECRET_TOKEN`, `DEV_MODE`, `AUTOUPDATER_ENABLED`, `AUTOUPDATER_CRON`, `COMPOSE_FILE` and `FILESHIP_URL`.
- Instance settings (site name, link protocol, uploads, trash retention, registration, Turnstile, AI, log level, telemetry, active storage) are stored in the `Setting` table and edited by super admins at `/dashboard/instance`. The env/runtimeConfig values are only their defaults. Server code reads them with `getSettings()` (or `getSettingsSync()` in synchronous code) from `server/utils/settings.ts`; the client reads the public subset with `usePublicSettings()`, kept in sync over the `settings:update` socket event. The schema lives in `shared/utils/settings.ts`.
- Server code reads the remaining configuration through `useRuntimeConfig()`, never `process.env`. Runtime overrides therefore use the `NUXT_` prefix. The exceptions are `DATABASE_URL` (shared with the Prisma CLI) and the build-time options in `nuxt.config.ts` (`ENABLE_PWA`, `DELETE_EXPIRED_CRON`, `GENERATE_STATS_CRON`), which only take effect when the app is built.
- Never commit `.env` files, `data/` or anything under `.claude/` or `.impeccable/`.

## Deployment model

`docker-compose.yml` runs `postgres`, a Caddy `proxy` (`caddy.json`), `fileship-blue`, `fileship-green` (profile `green`) and `updater-agent`.

- Caddy uses the `first` selection policy with active health checks on `/api/healthz`, so traffic goes to the first healthy color.
- An update pulls the image for the idle color, starts it, waits for `ready: true`, and then stops the old color. If the new color exits, dies or never becomes healthy, the agent stops it again and the old color keeps serving.
- `/api/healthz` returns `{ status, version, ready, busy }`. `busy` is true while any backup restore is running; the agent waits for it (up to 10 minutes) before stopping the old color, and the auto-updater skips a run when busy.
- Both colors share the database and `data/`. Startup work must be idempotent and safe to run twice (for example, `resetBackupRestoreState` only clears restores older than 2 hours, and `storage:sync` skips a storage that cannot be listed or looks empty).
- `docker-compose.dev.yml` is the same stack built from local Dockerfiles.

## Dashboard layout (`apps/dashboard`)

- `app/pages`: route groups `(auth)` (login, register), `(dashboard)/dashboard/*` (home, files, folders, notes, links, trash, logs, stats, users, instance, settings/\*) and `(view)` (public `view/[id]`, `folder/[id]`, `note/[id]`, `inbox/[id]`, `link/[code]`).
- `app/layouts`: `default.vue` (public pages) and `dashboard.vue` (sidebar shell, command palette, upload sheet and queue, global drop zone, paste upload).
- `app/middleware`: `userOnly`, `guestOnly`, `adminOnly`, `superAdminOnly`, `fileUploaderOnly`, `noteTakerOnly`, `linkShortenerOnly`.
- `app/components/ui`: shadcn-vue components (style `reka-luma`, see `components.json`). App components sit flat in `app/components`.
- `app/composables`: `useState` stores plus action composables (`useFileActions`, `useFolderActions`, `useNoteActions`).
- `app/utils`: client helpers (`socket.ts`, `uploadFile.ts`, `getErrorMessage.ts`, ...).
- `server/api`: REST endpoints (`auth`, `files`, `folders`, `inbox`, `links`, `logs`, `notes`, `settings`, `storages`, `trash`, `updater`, `users`, `stats`, `healthz`).
- `server/routes/u/[id]`: raw file serving. `server/routes/s/[code]`: short link redirects.
- `server/middleware`: `auth.ts` (the `token` session cookie, plus the upload token on `POST /api/files`) and `cors.ts`.
- `server/plugins`: `setup.ts`, `socketIO.ts`, `aiQueueRunner.ts`.
- `server/tasks`: `db:*`, `dir:*`, `search:*`, `ai:*`, `media:*`, `storage:*`, `telemetry:*`, scheduled in `nuxt.config.ts` or started from `setup.ts`.
- `server/storage`: storage drivers (`local`, `s3`, `sftp`, `ftp`) behind the `StorageDriver` interface in `types.ts`.
- `server/utils`: list/query helpers (`list.ts`), permissions, Prisma, Orama search, AI queue, sessions (`session.ts`), thumbnails and metadata (`media.ts`), storage (`storage.ts`, `upload.ts`, `backup.ts`, `fileData.ts`), `verifySession`, `createLog`, `errors.ts`, ...
- `shared/utils`: code used by both sides (constants, file categories, permissions, types, accents).
- `prisma/schema.prisma` and `prisma/migrations`. The client is generated into `generated/prisma` (gitignored).

## Key building blocks

Reuse these instead of writing new versions.

- `usePaginatedList(resource, endpoint, params, options)` (`app/composables/usePaginatedList.ts`): infinite list state (`items`, `total`, `summary`, `isLoading`, `hasMore`, `loadMore`, `refresh`). Socket events update every open list through `refreshLists`, `patchListItem`, `removeListItems` and `mapListItems`. `useListHandle` registers single items for detail pages.
- `DataTable.vue`: window-virtualized table. Columns support `sortable` and `descFirst`; it switches to a `#card` slot below 640px of container width and emits `reachEnd`.
- `VirtualList.vue`: window-virtualized responsive grid (`minItemWidth`, `gap`, `estimateSize`, `reachEnd`).
- `useQueryState(key, default)`: a ref bound to the URL query. It drops defaults and batches updates.
- `askConfirmation({ ..., action })` (`useConfirm.ts`): confirmation dialog that keeps a spinner until `action` resolves.
- `withVerification(action)` (`useVerification.ts`): retries an action after a password, TOTP or passkey prompt when the API asks for verification.
- `getErrorMessage(error)`: user-facing message from a fetch error.
- Storage: never build `data/uploads` or `data/backups` paths. Upload and backup bytes live on a storage (`File.storageId` / `Backup.storageId`, `null` is the data directory) under the keys `uploadKey(fileName)` (`uploads/<name>`) and `backupKey(backup)` (`backups/<userId>/<id>.tgz`). Use `getStorage(storageId)` for `read`, `put`, `move`, `remove` and `list`, and `withLocalFile(storageId, key, fn)` when a tool needs a real path (sharp, ffmpeg, AI, tar). `removeFileData(files)` removes a file's bytes, thumbnail and search entry.
- Chunked uploads go through `receiveUpload` (files, inbox) or `receiveChunk` (backups) in `server/utils/upload.ts`. Chunks are written straight to the active storage, except images that need EXIF removal or compression, which are assembled in `data/temp` first. Upload sessions live in `data/temp/<owner>/<uploadId>.upload.json` and `dir:clearTemp` aborts stale ones.
- Trash: `File`, `Folder` and `Note` have `deletedAt`. Every query that lists, counts, serves or edits them must add `deletedAt: null` (use `userCountSelect` from `list.ts` for user counts); only the trash endpoints and `server/utils/trash.ts` (`deleteTrashed`, `delete*Forever`, `filesTrashedWith`) work on trashed rows. User deletes go to the trash while `trashRetentionDays > 0`; expiry, max views, `storage:sync`, user deletion and backup restores delete immediately. `db:purgeTrash` runs hourly.
- Links: `Link` rows are served by `/s/[code]` through `findActiveLink` and `countLinkClick` (`server/utils/link.ts`). Owner clicks are not counted, a link is deleted at its click limit, and expired links are removed by `db:deleteExpired`.
- `storage:sync` runs on startup and removes files and backups whose bytes are missing, and indexes backup archives that have no row. `storage:migrate` moves everything to the active storage.
- AI jobs (`AIJob`) are queued with `enqueueAIJob` and run by `ai:processQueue`; `ai:syncJobs` queues what is missing on startup and when AI is turned on. Images get CLIP, caption and OCR jobs on upload; the OCR job queues the text embedding and `DetectPII`, and the caption job queues the text embedding again, which embeds caption + OCR. Text files get text embedding and `DetectPII` directly. Models live in `server/utils/ai.ts` (CLIP ViT-B/32 images with the multilingual CLIP text encoder for queries, Florence-2, multilingual MiniLM, NER); changing a model means a migration that clears the stored embeddings.
- Sensitive content: `server/utils/pii.ts` holds the validated patterns (`findPii`) and the card and ID document rules for OCR text (`findDocumentPii`); reason codes and their labels are in `shared/utils/pii.ts`. A name (`PERSON_NAME`) only counts next to another reason. `File.piiOverride` is the owner's choice (`PATCH /api/files/[id]` with `sensitive`); `DetectPII` never changes a file that has one, and `null` hands it back to the model.
- Search index: write file entries with `upsertFileSearchEntry(fileId)` (`server/utils/search.ts`), which re-reads the row and serializes writes, never with Orama `insert`/`update` directly. `searchFileIds` runs fulltext only for visitors, when AI is off for the instance or the user, or in fulltext mode; smart mode fuses CLIP, MiniLM and whole-word fulltext lists, each cut by a calibrated floor and a ratio of its best score. Re-calibrate those constants with English and Turkish queries when a model changes.
- `app/utils/socket.ts`: socket.io client that keeps user, theme, embed, domains, passkeys and lists in sync.

## Conventions

- Code style: no comments, `handle*` names for event handlers, `<template>` before `<script setup>`, `definePageMeta` last, non-null `!` and `as any` are fine where the type is known.
- Prettier: 4 spaces, single quotes, semicolons, trailing commas, width 100. Format only the files you changed.
- Create a component only when it is used in more than one place.
- Errors: `createError({ status, statusText?, message, data? })`. `statusCode` and `statusMessage` are deprecated.
- Long lists use virtual scroll with `reachEnd`, never pagination controls.
- Commits: conventional commits (`feat`, `fix(ui)`, `fix(api)`, `chore`, `docs`, ...), one topic per commit.

## UI rules

- Use shadcn-vue components. Read `https://www.shadcn-vue.com/docs/components/<name>` before using one and follow its documented composition.
- Never restyle `app/components/ui`. Small upstream logic or attribute fixes are acceptable; colors, radii and spacing are not changed there. Adjust at the usage site instead.
- Add new shadcn components from the `reka-luma` registry and keep them unmodified.
- No `cursor-pointer`.
- Icons come from lucide through `nuxt-lucide-icons` (`<IName />`).
- Use theme tokens (`bg-muted`, `text-muted-foreground`, `var(--primary)`, ...) instead of raw colors, so accents and dark mode keep working.

## Security notes

- `/u/[id]` serves user uploads with `X-Content-Type-Options: nosniff` and `Content-Security-Policy: sandbox`; HTML, SVG, XML and JavaScript are served as attachments.
- Orama indexes are shared; every search must filter by `authorId`.
- Public folders list password protected files to visitors only as `{ id, fileName, mimeType, size, createdAt, isLocked: true }`, matched by name only; their bytes need the password (`GET /api/files/[id]` is rate limited per IP and file). Sensitive files (`piiDetected`) are never shown to visitors, and folder archives leave out locked and sensitive files.
- Backup restores accept an explicit allow-list of fields per model and always force the owner to the current user.
- Upload ids, file names and paths are validated before touching the filesystem.
- Sensitive account actions go through `verifySession` on the server and `withVerification` on the client.
- Sessions are random tokens stored as SHA-256 hashes (`Session.tokenHash`) and sent only in the httpOnly `token` cookie. The client never reads or writes auth cookies. Sessions have no expiry and end only when they are revoked. Impersonation runs on the server (`/api/auth/impersonate`), with the admin's token kept in the httpOnly `adminToken` cookie.
- Storage configs are stored AES-GCM encrypted with the same `<data>/secret.key`. The API never returns secret fields; it sends `<field>: true/false` flags, and an empty secret on update keeps the stored value.
- Upload tokens (`fs_…`) are accepted only on `POST /api/files` and `POST /api/links`, through the `Authorization` header. They are stored hashed for lookup and AES-GCM encrypted for display, with the key in `<data>/secret.key`.
- Login, passkey login, registration and step-up verification are rate limited (`rateLimit`). TOTP codes cannot be reused (`verifyTotpToken`). Cookie-authenticated writes are rejected when the browser marks them as cross-site (`Sec-Fetch-Site` other than `same-origin` or `none`). The check does not depend on the `Host` header, so it works behind any reverse proxy.
- Short links only accept `http` and `https` targets. Password protected links redirect to `/link/[code]`, and `POST /api/links/unlock` is rate limited.
- Markdown is rendered without raw HTML, and links are limited to `http`, `https`, `mailto` and relative URLs.
