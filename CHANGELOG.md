# Changelog

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
