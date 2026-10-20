# MEMORY.md

Project knowledge that is not obvious from the code: decisions, release facts and the traps people (and AI agents) have already fallen into. [AGENTS.md](AGENTS.md) describes the architecture and conventions; [PLAN.md](PLAN.md) tracks what is left before v6. When you learn something here the hard way, add it.

## Branches and versions

- `main` is the released line, 6.x since v6.0.0. Every major version also keeps a branch named after it (`v1` … `v5`, `v6`); the v6 rewrite was developed on `v6-nextgen`, which became `v6` when 6.0.0 shipped.
- The root `package.json` is the Fileship version that `/api/healthz` reports. The desktop app requires Fileship 6.0.0 or newer (`MIN_FILESHIP_VERSION`).
- The desktop app has its own version in `apps/desktop/package.json` (1.0.0 shipped with Fileship 6.0.0). Auto-update never moves to a lower version.
- 5.x installations with the auto-updater on update themselves to the latest release, because the 5.x updater pulls `:latest` whenever the newest GitHub release is a higher version. A release must therefore always migrate a 5.x database and `data` folder; the 5.4.4 → 6.0.0 path was tested in Docker.

## Decisions

- v6 is a major version: breaking changes are fine and backward-compatibility shims are not wanted (old cookies, formats, endpoints). Mark such commits with `!` (`feat(auth)!: ...`) and a `BREAKING CHANGE:` footer.
- The only compatibility kept on purpose: the telemetry panel still accepts `POST /api/event` from pre-6.0 instances.
- Write code in the existing style: minimal, `!` instead of defensive `?.`/`??` where the value is known, no guards for states that cannot happen, no comments. Mirror `AppSidebar.vue`, `layouts/dashboard.vue` and `login.vue` in the dashboard.
- shadcn-vue components are used as documented and never restyled; lists use virtual scroll, never pagination; no `cursor-pointer` anywhere.
- Telemetry panel: admin only (one password), default Fileship colors only, instances silent for `NUXT_INSTANCE_RETENTION_DAYS` (default 90) are deleted while daily totals stay. The legacy import from the old panel is done and was removed on purpose; do not bring it back.
- Translations: English is the source and fallback; Turkish, Simplified Chinese, German, French and Italian follow it. The telemetry panel is not translated on purpose (it gets the layer's English messages only and keeps `pluralize`). Turkish copy is written as native product text in the "sen" form, never word by word; see [I18N.md](I18N.md).
- The desktop app has no separate image editor. It was removed on purpose because the region overlay already annotates while selecting; do not bring it back.
- Desktop capture follows ShareX: Print Screen captures the active monitor, Ctrl+Print Screen opens the region overlay, a selection finishes on mouse release, annotations can be drawn before choosing the region, and one toolbar serves every monitor.
- The drive is called **Fileship Harbor** ("Harbor" for short) in the UI and the docs, in every language. Internal names stay `drive` (code, settings keys, `/settings/drive`, i18n keys, `fileship-drive.exe`, sync root ids) so existing installs keep working.

## Releasing

- **Desktop:** bump `apps/desktop/package.json`, push the branch, then push a tag `desktop-v<version>` that matches it exactly. `.github/workflows/desktop-release.yml` runs only on such tags (there is no "Run workflow" button until the file is on `main`). It creates a draft release in the public `hanzydev/fileship-desktop` repo, lets electron-builder upload into it and publishes it. The `DESKTOP_RELEASE_TOKEN` secret is a token with Contents read/write on `fileship-desktop`.
- Do not let electron-builder 26 create the release itself: it creates one publisher per artifact before caching it, so the installer and its blockmap both try to create the release and the second fails with `422 Published releases must have a valid tag`. That is why the workflow makes the draft first.
- electron-builder only uploads into an already published release within 2 hours of its publication. To redo an older release, delete the release and its tag in `fileship-desktop` and push the Fileship tag again on a commit with the current workflow (a re-run uses the workflow file of the tagged commit).
- **Docker images** (`docker-publish.yml`) are built for `fileship`, `fileship-updater-agent` and `fileship-telemetry` when a GitHub release is published in the Fileship repo.
- CI (`check.yml`) runs `pnpm check` on `ubuntu-latest`. If the Ubuntu 26 image breaks something, pin `ubuntu-24.04`.

## Build and tooling traps

- **Workspace packages and Nitro:** Nitro inlines workspace packages (their real path is outside `node_modules`) and only copies traced files from `node_modules`. Anything a package loads by path at runtime, such as the AI worker, has to be copied into `.output` by the `compiled` hook in `apps/dashboard/nuxt.config.ts`.
- **Config files must load on a fresh clone.** `pnpm install` runs `nuxt prepare` (postinstall) before any workspace package is built, so `nuxt.config.ts` must not `require.resolve` files from a `dist/` folder. Build paths from the package root instead (`@fileship/ai/package.json` is exported for that).
- Workspace packages (`packages/ai`, `packages/telemetry`) are built with tsdown. `pnpm dev` builds them first through Turbo (`^build`); after changing one by hand, run `pnpm --filter @fileship/ai build`.
- **Packaging the desktop app:** run `pnpm build` right before electron-builder. A stale `dist-electron` (for example from a `tsdown --watch` dev process) gets packaged otherwise.
- On Windows, files in `apps/desktop/release` can be locked by Explorer, an editor or an AI desktop app that has the folder open, and electron-builder then fails with `EBUSY`. Build into a folder outside the repo with `--config.directories.output=<path>` and copy the result.
- In Git Bash, `node -e` and heredocs mangle template literals (`${...}`) and backticks. Write a script file instead.
- `LF will be replaced by CRLF` warnings are harmless.
- Run prettier only on the files you changed; `pnpm lint` formats the whole repository.

## Runtime traps

- **Server translations load one locale.** `useTranslation(event)` only loads the messages of the detected locale, so a key missing in `tr` comes back as the raw key instead of falling back to English. `pnpm lint:i18n` keeps the catalogs equal; never skip it.
- **vue-i18n message syntax:** `@`, `$`, `|`, `{` and `}` are syntax. Literal ones are written `{'@'}` (an email placeholder needs it). The check script compiles every message to catch this.
- **Translated messages are not identifiers.** Client code that compared `getErrorMessage(error)` with an English string broke as soon as the API translated it; the API sends a flag in `data` (for example `totpRequired`).

- **AI never runs on the main thread.** onnxruntime runs inference and session creation synchronously, so models on the main thread freeze every request, socket message and health check for seconds (and Caddy's 2 s health check then fails). Everything goes through `packages/ai` and its worker. To measure lag, poll `/api/healthz` every 100 ms while files are being analyzed; p95 should stay well under 100 ms.
- **Orama:** the default English splitter drops every non-Latin character. Every index uses `createSearchTokenizer`; never create an Orama index without it.
- **Hydration:** `TextBlock` renders a `<p>`. Wrapping block content in it (for example `UiSkeleton`, a `<div>`) causes hydration mismatches; use `custom-tag="div"`.
- **Dialog exit animations:** if a dialog's content is rendered with `v-if` on the same state that closes it, the content disappears at once and the close animation never plays. Keep a copy of the last item and render from that (see `layouts/dashboard.vue` and the public folder page).
- **Client IPs in development:** the dev server has no proxy, so the peer address is empty and `getClientIp` trusts the forwarding headers. Test scripts can send `X-Fileship-Client-IP: 10.x.y.z` with a different private address per run to avoid the login rate limit (10 attempts per 15 minutes per IP and username).

## Testing the desktop app safely

- The dev app and the installed app share one profile (`%APPDATA%\Fileship`) and a single-instance lock. Run a test instance with its own `--user-data-dir`, for example: start `pnpm exec nuxt dev --port 3020` and `npx tsdown` in `apps/desktop`, then run Electron from `apps/desktop` with `DESKTOP_RENDERER_URL=http://localhost:3020` and `npx electron . --user-data-dir=<scratch folder> --remote-debugging-port=<port>`.
- In a test profile, turn global hotkeys off (`hotkeysEnabled: false`), or the test instance takes Print Screen away from the real app. Keep the drive off, and turn uploads off (`afterCapture.upload`) unless the test needs them.
- Log test instances in with a dedicated test account on a local dashboard, never with a real account.
- Packaged builds refuse `--remote-debugging-port` and close DevTools. Use CDP against a dev instance.
- The capture overlay covers every monitor, so the real mouse moving or clicking during an automated test cancels synthetic drags or captures a window. Keep visible tests short and always close the overlay with Escape.
- Never point a test drive at `%USERPROFILE%\Fileship` or another real folder. Note that Windows does not report sync roots under `%TEMP%` through `GetCurrentSyncRoots` or `GetSyncRootInformationForFolder`, even though registering them works.

## Drive (Cloud Files API) facts

- Each account registers its own sync root `Fileship!<SID>!<account key>` (the key is a hash of the instance URL and user id). A folder can belong to only one sync root; registering it again under another id fails with `Access is denied`. The helper therefore unregisters a stale Fileship root of the same Windows user on the same folder before registering.
- A failed registration can leave an empty key under `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\SyncRootManager`; it is harmless and is filled in by the next successful registration.
- `fileship-drive.exe --unregister-all` removes every Fileship sync root that Windows lists (the uninstaller runs it).
- Shell handlers (thumbnails, and later custom states, verbs, sharing) are COM classes hosted by the helper itself (`native/drive/com.cpp`): per user under `HKCU\Software\Classes\CLSID\{clsid}\LocalServer32`, and named in the sync root key (`ThumbnailProvider` = `{clsid}`, written after every `Register()`; `Unregister()` deletes the whole key with our values). When Fileship is closed, COM starts `fileship-drive.exe -Embedding`, which reads `%LOCALAPPDATA%\Fileship\harbor.json` (token DPAPI protected) and exits after 30 s idle. Whether an already running Explorer picks up a new `ThumbnailProvider` without a restart is not verified yet.
- `RecycleBinUri` is stored as `RecycleBinUrl` in the sync root key; `ProviderId` does not show up there.
- Processes started from Claude Code's shell get `REGDB_E_CLASSNOTREG` for every per-user `LocalServer32`, so COM never launches the helper from there and Shell.Application sees no Explorer windows. Test `-Embedding` by starting it by hand and connecting with `CoCreateInstance`.

## Working with AI agents

- `CLAUDE.md` imports `AGENTS.md`, `MEMORY.md` and `PLAN.md`; other tools read `AGENTS.md`, which points here.
- Agents commit with conventional commits, one topic per commit, and do not push, tag or publish unless asked. A tag push publishes a public desktop release, so treat it like a release.
- Agents never touch real accounts, real drive folders, `.env` files or secrets, and never run destructive database commands. Test accounts and test data are created for the task and removed afterwards.
- Parallel agents in one checkout must own separate files, stage only their own paths (`git add <paths>`, never `-A`) and re-read a file before editing it. Files that many tasks touch (`overlay.vue`, `useAnnotator.ts`, `nuxt.config.ts`) belong to one agent at a time.
- Keep AGENTS.md accurate: when a change alters a documented behaviour, update the paragraph in the same commit.
- Keep the README short on purpose. It once grew to about 450 lines and nobody read it; user-facing details belong in `docs/`.
