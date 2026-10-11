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
- Self-registration is off by default. `NUXT_PUBLIC_REGISTRATION_ENABLED=true` enables `/register` and `POST /api/auth/register`; `NUXT_REGISTRATION_PERMISSIONS` (only `UploadFiles` and `TakeNotes` are honoured) and `NUXT_REGISTRATION_USABLE_SPACE` (MB, `-1` unlimited) set what new accounts get.
- `NUXT_UPDATER_URL` and `NUXT_UPDATER_SECRET` point the dashboard at the updater-agent. `docker-compose.yml` sets both.
- The updater-agent reads `PORT`, `SECRET_TOKEN`, `DEV_MODE`, `AUTOUPDATER_ENABLED`, `AUTOUPDATER_CRON`, `COMPOSE_FILE` and `FILESHIP_URL`.
- Instance settings (site name, link protocol, uploads, registration, Turnstile, AI, log level, telemetry) are stored in the `Setting` table and edited by super admins at `/dashboard/instance`. The env/runtimeConfig values are only their defaults. Server code reads them with `getSettings()` (or `getSettingsSync()` in synchronous code) from `server/utils/settings.ts`; the client reads the public subset with `usePublicSettings()`, kept in sync over the `settings:update` socket event. The schema lives in `shared/utils/settings.ts`.
- Server code reads the remaining configuration through `useRuntimeConfig()`, never `process.env`. Runtime overrides therefore use the `NUXT_` prefix. The exceptions are `DATABASE_URL` (shared with the Prisma CLI) and the build-time options in `nuxt.config.ts` (`ENABLE_PWA`, `DELETE_EXPIRED_CRON`, `GENERATE_STATS_CRON`), which only take effect when the app is built.
- Never commit `.env` files, `data/` or anything under `.claude/` or `.impeccable/`.

## Deployment model

`docker-compose.yml` runs `postgres`, a Caddy `proxy` (`caddy.json`), `fileship-blue`, `fileship-green` (profile `green`) and `updater-agent`.

- Caddy uses the `first` selection policy with active health checks on `/api/healthz`, so traffic goes to the first healthy color.
- An update pulls the image for the idle color, starts it, waits for `ready: true`, and then stops the old color. If the new color exits, dies or never becomes healthy, the agent stops it again and the old color keeps serving.
- `/api/healthz` returns `{ status, version, ready, busy }`. `busy` is true while any backup restore is running; the agent waits for it (up to 10 minutes) before stopping the old color, and the auto-updater skips a run when busy.
- Both colors share the database and `data/`. Startup work must be idempotent and safe to run twice (for example, `resetBackupRestoreState` only clears restores older than 2 hours).
- `docker-compose.dev.yml` is the same stack built from local Dockerfiles.

## Dashboard layout (`apps/dashboard`)

- `app/pages`: route groups `(auth)` (login, register), `(dashboard)/dashboard/*` (home, files, folders, notes, logs, stats, users, instance, settings/\*) and `(view)` (public `view/[id]`, `folder/[id]`, `note/[id]`, `inbox/[id]`).
- `app/layouts`: `default.vue` (public pages) and `dashboard.vue` (sidebar shell, command palette, upload sheet and queue, global drop zone, paste upload).
- `app/middleware`: `userOnly`, `guestOnly`, `adminOnly`, `superAdminOnly`, `fileUploaderOnly`, `noteTakerOnly`.
- `app/components/ui`: shadcn-vue components (style `reka-luma`, see `components.json`). App components sit flat in `app/components`.
- `app/composables`: `useState` stores plus action composables (`useFileActions`, `useFolderActions`, `useNoteActions`).
- `app/utils`: client helpers (`socket.ts`, `uploadFile.ts`, `getErrorMessage.ts`, ...).
- `server/api`: REST endpoints (`auth`, `files`, `folders`, `inbox`, `logs`, `notes`, `settings`, `updater`, `users`, `stats`, `healthz`).
- `server/routes/u/[id]`: raw file serving.
- `server/middleware`: `auth.ts` (session cookie) and `cors.ts`.
- `server/plugins`: `setup.ts`, `socketIO.ts`, `aiQueueRunner.ts`.
- `server/tasks`: `db:*`, `dir:*`, `search:*`, `ai:*`, `telemetry:*`, scheduled in `nuxt.config.ts`.
- `server/utils`: list/query helpers (`list.ts`), permissions, Prisma, Orama search, AI queue, `verifySession`, `createLog`, `errors.ts`, ...
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
- Backup restores accept an explicit allow-list of fields per model and always force the owner to the current user.
- Upload ids, file names and paths are validated before touching the filesystem.
- Sensitive account actions go through `verifySession` on the server and `withVerification` on the client.
