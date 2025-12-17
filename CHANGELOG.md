# Changelog

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
