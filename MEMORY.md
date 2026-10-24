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
- Translations: English is the source and fallback; Turkish, German, French, Italian, Azerbaijani, Japanese, Korean and Simplified Chinese follow it. The telemetry panel is not translated on purpose (it gets the layer's English messages only and keeps `pluralize`). Turkish copy is written as native product text in the "sen" form, never word by word; see [I18N.md](I18N.md).
- The desktop app has no separate image editor. It was removed on purpose because the region overlay already annotates while selecting; do not bring it back.
- Desktop capture follows ShareX: Print Screen captures the active monitor, Ctrl+Print Screen opens the region overlay, a selection finishes on mouse release, annotations can be drawn before choosing the region, and one toolbar serves every monitor.
- The Explorer integration is called **Fileship Harbor** ("Harbor" for short) in the UI and the docs, in every language, and `harbor` everywhere in the project (code, settings keys, `/settings/harbor`, i18n keys, IPC channels, `--harbor-action`, `fileship-harbor.exe`). 1.0.1 stored `settings.drive` and `driveRoot`; `migrateHarbor` in `electron/store.ts` moves them over when the store loads.

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
- In Git Bash, `node -e` and heredocs mangle template literals (`${...}`), backticks and backslashes (double backslashes can arrive halved). Write a script file instead, or edit such lines with an editor tool.
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
- **Two copies of Vue in dev:** when Vite re-optimizes dependencies while a page is open, the page can load Vue twice (different `?v=` hashes on `runtime-core`) and fail with `Cannot read properties of null (reading 'ce')` in `renderSlot`. Reload the page; it is not a code bug.
- **Cookies ignore ports.** Several dashboards on `localhost` (parallel agents, other checkouts) overwrite each other's `token` cookie in a shared browser, which looks like a random logout (a sudden 401 on `/api/users/@me`). Test APIs with `Authorization: Bearer` tokens and log in again when the browser session drops.
- **Client IPs in development:** the dev server has no proxy, so the peer address is empty and `getClientIp` trusts the forwarding headers. Test scripts can send `X-Fileship-Client-IP: 10.x.y.z` with a different private address per run to avoid the login rate limit (10 attempts per 15 minutes per IP and username).

## Testing the desktop app safely

- Agents cannot close the installed Fileship themselves: the tray app ignores `taskkill` without `/F`, and force-closing it is blocked as interfering with the user's work. A Harbor test needs the user to quit Fileship from the tray first.
- The dev app and the installed app share one profile (`%APPDATA%\Fileship`) and a single-instance lock. Run a test instance with its own `--user-data-dir`, for example: start `pnpm exec nuxt dev --port 3020` and `npx tsdown` in `apps/desktop`, then run Electron from `apps/desktop` with `DESKTOP_RENDERER_URL=http://localhost:3020` and `npx electron . --user-data-dir=<scratch folder> --remote-debugging-port=<port>`.
- In a test profile, turn global hotkeys off (`hotkeysEnabled: false`), or the test instance takes Print Screen away from the real app. Keep Harbor off, and turn uploads off (`afterCapture.upload`) unless the test needs them.
- Log test instances in with a dedicated test account on a local dashboard, never with a real account.
- Packaged builds refuse `--remote-debugging-port` and close DevTools. Use CDP against a dev instance.
- The capture overlay covers every monitor, so the real mouse moving or clicking during an automated test cancels synthetic drags or captures a window. Keep visible tests short and always close the overlay with Escape.
- Never point a test Harbor at `%USERPROFILE%\Fileship` or another real folder. Note that Windows does not report sync roots under `%TEMP%` through `GetCurrentSyncRoots` or `GetSyncRootInformationForFolder`, even though registering them works. A new folder such as `%USERPROFILE%\FileshipHarborTest` works; logging the test instance out removes it.
- A Harbor test instance rewrites the installed app's COM registration (`HKCU\...\CLSID\{clsid}\LocalServer32`) and `%LOCALAPPDATA%\Fileship\harbor.json`, and both helpers would host the same CLSIDs. Quit the installed Fileship first and start it again afterwards; its `init` writes both back.
- `node scripts/dev.mjs --user-data-dir=<profile>` with `DESKTOP_DEBUG_PORT=<port>` runs a dev instance with CDP; its electron-log output goes to the script's stdout, not `%APPDATA%\Fileship\logs`. The renderer drives IPC (`window.fileship.invoke`), and `npx electron . --user-data-dir=<same profile> --harbor-action <action> <path>` reaches the running instance like an Explorer verb. Chromium moves switches before positional arguments, so in dev the app path `.` comes right after `--harbor-action`; `handleLaunch` skips it.
- The flyout can be read and its commands invoked from a small C++/WinRT client: `create_instance<IStorageProviderStatusUISourceFactory>(CLSID_FileshipStatus, CLSCTX_LOCAL_SERVER)`, then `GetStatusUISource(L"").GetStatusUI()`.
- On an empty local database `db:createDefaultUser` creates the seed super admin `admin` / `password`; it can create test users through `POST /api/users`.
- `nuxt typecheck` in the dashboard fails on `@fileship/ai` and `@fileship/telemetry` until they are built (`pnpm --filter @fileship/ai --filter @fileship/telemetry build`). In a fresh checkout the telemetry panel also needs its Prisma client (`pnpm --filter telemetry db:generate`), or `pnpm lint:types` reports `unknown` types in its pages.
- Editing catalogs while the dashboard dev server runs rebuilds Nitro, which can crash the process with `0xC0000409` while the AI worker is being stopped. Restart it; it is not caused by the change.

## Harbor (Cloud Files API) facts

- Each account registers its own sync root `Fileship!<SID>!<account key>` (the key is a hash of the instance URL and user id). A folder can belong to only one sync root; registering it again under another id fails with `Access is denied`. The helper therefore unregisters a stale Fileship root of the same Windows user on the same folder before registering.
- A failed registration can leave an empty key under `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\SyncRootManager`; it is harmless and is filled in by the next successful registration.
- `fileship-harbor.exe --unregister-all` removes every Fileship sync root that Windows lists (the uninstaller runs it).
- Shell handlers (thumbnails, `ShareHandler`, the status flyout) are COM classes hosted by the helper itself (`native/harbor/com.cpp`): per user under `HKCU\Software\Classes\CLSID\{clsid}\LocalServer32`, and named in the sync root key (`ThumbnailProvider` = `{clsid}`, written after every `Register()`; `Unregister()` deletes the whole key with our values). When Fileship is closed, COM starts `fileship-harbor.exe -Embedding`, which reads `%LOCALAPPDATA%\Fileship\harbor.json` (token DPAPI protected) and exits after 30 s idle. Whether an already running Explorer picks up a new `ThumbnailProvider` without a restart is not verified yet.
- `RecycleBinUri` is stored as `RecycleBinUrl` in the sync root key; `ProviderId` does not show up there.
- `attrib +P` on a folder does not pin its children, and the watcher never sees the placeholders the helper creates itself. The helper pins the children of a pinned folder (`pinChildren`) and pins and hydrates new placeholders in a pinned folder (`inheritPin`).
- Processes started from Claude Code's shell get `REGDB_E_CLASSNOTREG` for every per-user `LocalServer32`, so COM never launches the helper from there and Shell.Application sees no Explorer windows. Test `-Embedding` by starting it by hand and connecting with `CoCreateInstance`.
- Custom state definitions land in `<sync root key>\CustomStates\<id>\DisplayName` (a plain string, so names are translated when the root is registered). `StorageProviderItemProperties::SetAsync` works on dehydrated placeholders without hydrating them, replaces the whole list (an empty list clears it), and reads back only as the `System.StorageProviderCustomStates` blob (a serialized property store with the values and icon resources); `System.ItemCustomState.*` stay empty outside Explorer.
- Verb `AppliesTo` with `System.ItemPathDisplay:~<"<root>\"` is a prefix match and `NOT …` works; Shell.Application `Verbs()` evaluates it in process (also on placeholders), so it can be tested from a script. Keep the trailing backslash, or a sibling folder such as `<root>-old` matches too.
- The Windows 11 flyout draws `ProviderSecondaryCommands` as icon buttons and skips commands whose `Icon()` is null, so every command carries an SVG (`native/harbor/icons/<light|dark>/`, shipped as `resources/harbor/icons`, picked by the `UISettings` foreground color like the CloudMirror sample). A plain `C:...` path is a valid `Uri` (it becomes `file:///`).
- Windows SDK 10.0.26100 has `IStorageProviderStatusUISourceFactory`. Explorer calls the status source (the flyout with **View online** and **Settings** shows in a real Explorer). The quota, more info and sync status sections and the five buttons were tested only through `CoCreateInstance`; how Explorer draws them, an empty `MoreInfoUI` command and the unlimited quota (`QuotaTotalInBytes = UINT64_MAX`, empty bar) is not verified yet. `QuotaUI` stays hidden without a `QuotaUsedLabel`, and the docs say Windows may disable the flyout without a `SyncStatusCommand`.
- `GetStatusUI` runs whenever Explorer shows the provider's state, so the helper only reads the last `syncStatus` JSON and sends `statusRequested`; the app throttles the refresh.
- Writing a file with `CREATE_ALWAYS` (Node's `writeFileSync`, atomic saves) replaces the placeholder: its identity is gone and the watcher reports `localNew` on the same path, so the app must treat a `localNew` on a known path as an edit.
- `CfUpdatePlaceholder` with `CF_UPDATE_FLAG_DEHYDRATE` refuses pinned files; `refreshPlaceholder` unpins, updates and pins again, and the watcher hydrates it.
- A custom state with an empty `IconResource` (the Views column) is accepted by `SetAsync`; how Explorer shows it is not verified.

## Working with AI agents

- `CLAUDE.md` imports `AGENTS.md`, `MEMORY.md` and `PLAN.md`; other tools read `AGENTS.md`, which points here.
- Agents commit with conventional commits, one topic per commit, and do not push, tag or publish unless asked. A tag push publishes a public desktop release, so treat it like a release.
- Agents never touch real accounts, real Harbor folders, `.env` files or secrets, and never run destructive database commands. Test accounts and test data are created for the task and removed afterwards.
- Parallel agents in one checkout must own separate files, stage only their own paths (`git add <paths>`, never `-A`) and re-read a file before editing it. Files that many tasks touch (`overlay.vue`, `useAnnotator.ts`, `nuxt.config.ts`) belong to one agent at a time.
- Keep AGENTS.md accurate: when a change alters a documented behaviour, update the paragraph in the same commit.
- Keep the README short on purpose. It once grew to about 450 lines and nobody read it; user-facing details belong in `docs/`.
