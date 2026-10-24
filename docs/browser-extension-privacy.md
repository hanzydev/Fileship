# Fileship browser extension privacy policy

Last updated: October 24, 2026

The Fileship browser extension uploads files, images and screenshots to a Fileship server and shortens links with it. Fileship is self-hosted: the server is one that you run, or one run by whoever gave you your account. This page explains what the extension handles, where it goes and what stays on your computer.

## Who receives your data

Only the Fileship server whose address you enter in the extension's settings. The extension sends nothing to the Fileship developers, to Google, to Mozilla or to anyone else, and it has no analytics, tracking, advertising or crash reporting.

What happens to your uploads on that server is up to whoever runs it.

## What the extension sends, and when

The extension sends something only when you use one of its actions: the toolbar button, a right-click menu item or the upload page.

- **Files** you choose or drop are uploaded to your server.
- **Images** you right-click are downloaded from the site that shows them, the way the browser loads them, and uploaded to your server.
- **Screenshots** of the visible part of the current tab, or of an area you select, are uploaded to your server.
- **Links:** the address of a link or of the current tab is sent to your server when you choose to shorten it.
- **Your upload token** goes with these requests to your server, in the `Authorization` header. Testing the connection also reads your server's version and upload limits and checks what the token is allowed to do, without creating anything.

The extension does not read the pages you visit and sends nothing in the background.

## What stays on your computer

- In the browser's extension storage: your server address, your upload token, the result of the last connection test (server name, version and what the token can do) and your upload options (file name style and expiry).
- The list of recent uploads and short links (names, links and progress) is kept only for the browser session and cleared when the browser closes.
- Files you pick from the toolbar button are kept in the extension's own database until their upload ends, then deleted.

**Disconnect** in the extension's settings removes the token and the access to your server. Removing the extension removes everything it stored.

## Permissions

The extension asks for access to your server when you save the connection, and for access to another site only when you upload an image from it. Every permission and why it is needed is listed in the [browser extension guide](browser-extension.md#permissions).

## Sharing and selling

The Fileship developers do not collect, sell or transfer any user data. The use of information received through the extension adheres to the [Chrome Web Store User Data Policy](https://developer.chrome.com/docs/webstore/program-policies/user-data-faq), including the Limited Use requirements.

## Changes

Changes to this policy are published on this page, and its history is in the [repository](https://github.com/hanzydev/Fileship/commits/main/docs/browser-extension-privacy.md).

## Contact

Questions about this policy: open an issue at [github.com/hanzydev/Fileship/issues](https://github.com/hanzydev/Fileship/issues).
