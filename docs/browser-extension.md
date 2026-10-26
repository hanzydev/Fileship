# Browser extension

The Fileship browser extension uploads files, images and screenshots to your Fileship and shortens links, right from Chrome, Edge or Firefox. It talks to your own server with your upload token, nothing else.

## Features

- **Toolbar button:** upload files, shorten the current tab, capture the visible part of the page, or drag over the page to capture an area
- **Right-click menu:** **Upload image to Fileship** on images, **Shorten link with Fileship** on links and **Shorten page link with Fileship** anywhere on a page
- The link is copied to the clipboard and a notification shows it; click the notification to open the link
- The toolbar button lists recent uploads and short links with their progress, a copy button and errors in plain words (an invalid token, a full storage, a page that cannot be captured)
- Uploads use your default file name style (random, UUID or the original name) and expiry, and follow the chunk size and parallel uploads of your server, so large files work too
- English, Turkish, German, Spanish, French, Italian, Brazilian Portuguese, Russian, Azerbaijani, Japanese, Korean and Simplified Chinese, following the language of the browser

## Requirements

- Chrome, Edge or another Chromium browser (version 116 or newer), or Firefox 142 or newer
- A Fileship 6.0 server or newer, and an account that can upload files, shorten links or both

## Installing

- **Chrome and other Chromium browsers:** install [Fileship from the Chrome Web Store](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh). The browser keeps it up to date.
- **Edge:** open the same Chrome Web Store page, choose **Allow extensions from other stores** in the banner Edge shows, then **Add to Chrome**.
- **Firefox:** the extension is not on addons.mozilla.org yet. Build it from this repository (Node.js 22+ and pnpm):

    ```sh
    pnpm install
    pnpm --filter extension build
    ```

    Then open `about:debugging#/runtime/this-firefox`, choose **Load Temporary Add-on** and select `manifest.json` in `apps/extension/.output/firefox-mv3`. Temporary add-ons are removed when Firefox closes.

To try a build of your own in Chrome or Edge, run the same build, open `chrome://extensions` (or `edge://extensions`), turn on **Developer mode**, choose **Load unpacked** and select `apps/extension/.output/chrome-mv3`.

## Connecting

1. In the dashboard, open **Settings → Integrations** and copy your upload token.
2. Click the Fileship button in the toolbar and choose **Open settings** (or open the extension's options).
3. Enter your server address (for example `files.example.com`; `https://` is added when you leave it out) and the upload token.
4. Choose **Test connection** to check it, then **Save**. The browser asks to let the extension access your server; allow it.

The test shows your server's name and version and whether the token can upload files, shorten links or both. **Disconnect** removes the token and the access to your server.

The upload token is kept in the browser's extension storage on this computer only. It can only upload files and shorten links; if it leaks, create a new one with **Regenerate** on the Integrations page.

## Upload options

The options page also sets the defaults for everything uploaded from the browser:

- **File name:** a random name, a UUID or the original file name
- **Expiration:** never, or from 5 minutes to 1 year

Folders, passwords and view limits are not available, because an upload token cannot read your folders. Move files into folders in the dashboard.

## Permissions

The extension asks only for what it needs:

| Permission                     | Why                                                                                                      |
| ------------------------------ | -------------------------------------------------------------------------------------------------------- |
| Access to your Fileship server | Asked when you save the connection, so the extension can upload and shorten links there. Nothing else.   |
| Access to an image's site      | Asked the first time you upload an image from another site, so the extension can download it.            |
| `activeTab`                    | Read the address of the current tab and capture it, only after you click the toolbar button or the menu. |
| `scripting`                    | Show the area picker on the page when you choose **Capture an area**.                                    |
| `contextMenus`                 | The right-click menu items.                                                                              |
| `notifications`                | Show the link after an upload, or what went wrong.                                                       |
| `clipboardWrite`               | Copy the link.                                                                                           |
| `storage`                      | Keep the server address, the token, your upload options and the recent list.                             |
| `offscreen` (Chrome and Edge)  | Copy the link to the clipboard from the background.                                                      |

Images from the same site as the page are downloaded through the page itself and need no extra access. Pages such as the browser's own settings and the extension stores cannot be captured.

In Firefox, the add-on declares that it sends website content (images and screenshots) and browsing activity (the links you shorten) to the server you configure, and only when you ask it to.

The [privacy policy](browser-extension-privacy.md) explains what the extension sends, where it goes and what it keeps on your computer.

## Using it

- **Upload files:** click the Fileship button and choose **Upload files**. In Firefox this opens an upload tab, because Firefox closes the toolbar popup when a file picker opens; drop files there or choose them.
- **Shorten this page:** shortens the current tab (http and https pages only).
- **Capture visible area:** uploads a screenshot of what you see in the tab.
- **Capture an area:** drag over the page to choose the area. A click without dragging captures the visible page, and Esc or a right click cancels.
- **Right-click** an image, a link or the page for the matching menu item.

Uploads keep running after the toolbar popup closes. The recent list is cleared when the browser closes.

## Troubleshooting

- **The upload token is not valid:** copy the token again from **Settings → Integrations**; it changes when it is regenerated.
- **Your storage is full:** free up space or ask an admin for more usable space.
- **Your account cannot upload files / shorten links:** an admin can grant the permission on the users page.
- **The extension can no longer reach your server:** the access to the server was removed in the browser's extension settings. Save the connection again.
- **No Fileship server was found:** check the address; the extension needs Fileship 6.0 or newer.

## Building packages

```sh
pnpm --filter extension package
```

This writes the store packages to `apps/extension/.output`: `fileship-<version>-chrome.zip` (Chrome Web Store and Edge Add-ons), `fileship-<version>-firefox.zip` and `fileship-<version>-sources.zip` (the source code that addons.mozilla.org asks for).

### Rebuilding the Firefox package from its sources

addons.mozilla.org rebuilds the add-on from `fileship-<version>-sources.zip` and compares the result with the submitted package. Unzip it and run these commands in the unzipped folder, on Windows, macOS or Linux, with Node.js 22 or newer (tested with 24) and pnpm 11.9.0 (`corepack enable` picks the version set in `package.json`; `npm install -g pnpm@11.9.0` works too):

```sh
corepack enable
pnpm install --frozen-lockfile
pnpm --filter extension build
```

`apps/extension/.output/firefox-mv3` then holds exactly the files of `fileship-<version>-firefox.zip`. The sources include the repository's `.gitignore`, because Tailwind reads it to skip build folders when it looks for class names; without it the stylesheet comes out different.

`pnpm --filter extension dev` (or `dev:firefox`) starts a browser with the extension and reloads it on changes.
