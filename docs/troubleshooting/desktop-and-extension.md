---
title: Windows app and extension problems
description: Fix Fileship for Windows and the browser extension - server too old, unsupported Windows, invalid upload token, missing permissions and full storage.
---

# Windows app and extension problems

## Fileship for Windows

### The installer refuses to run

Fileship for Windows needs 64-bit Windows 11, version 24H2 (build 26100) or newer. Older Windows 11 releases and Windows 10 are not supported. Check your version with `winver`.

### Windows SmartScreen warns about the installer

The installer is not code signed yet. Choose **More info → Run anyway**.

### The server is too old

The app needs Fileship 6.0 or newer. Update your server (see [Updating](../updating.md)).

### Login doesn't finish

- Enter the server's public address, the same one you use in the browser.
- Behind single sign-on, the login runs in the app's own window; see [Single sign-on problems](single-sign-on.md).

### Uploads fail with "Your storage is full"

Your account reached its storage limit. Clicking the notification opens the storage page of the dashboard. A screenshot whose upload fails is saved to the save folder instead of being lost.

### Text capture says AI features are turned off

Text capture uses your server's OCR, which needs AI on for the server and your account. See [AI problems](ai.md#ai-features-are-turned-off).

### Harbor

See [Fileship for Windows](../desktop.md) for how Harbor works. Harbor needs an empty folder, and changes from the server arrive over the same live connection as the dashboard's ([Live updates don't work](realtime.md)).

## Browser extension

- **The upload token is not valid:** copy the token again from **Settings → Integrations**; it changes when it is regenerated.
- **Your storage is full:** free up space or ask an admin for more usable space.
- **Your account cannot upload files / shorten links:** an admin can grant the permission on the **Users** page.
- **The extension can no longer reach your server:** the access to the server was removed in the browser's extension settings. Save the connection again.
- **No Fileship server was found:** check the address; the extension needs Fileship 6.0 or newer.
- **Large uploads fail with 413:** your reverse proxy's body limit is smaller than the chunk size; see [Upload problems](uploads.md#upload-failed-with-413-request-entity-too-large).

See [Browser extension](../browser-extension.md) for permissions and setup.
