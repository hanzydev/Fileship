# Fileship on your phone

The dashboard works in any mobile browser. Installed as an app, it opens in its own window from the home screen and, on Android, shows up in the share sheet of other apps, so you can send photos, files, links and text straight to Fileship.

## Requirements

- Fileship must be served over HTTPS (see [Web server configuration](installation.md#web-server-configuration-optional)). Browsers only install apps and run their service worker on secure origins; `http://localhost` is the only exception.
- The app is built with `ENABLE_PWA` on, which is the default and what the published image uses. With `ENABLE_PWA=false` the dashboard has no web app manifest and no service worker, so it cannot be installed and nothing can be shared to it.

## Install the app

**Android (Chrome, Edge, Samsung Internet):** open your Fileship, log in, open the browser menu and choose **Install app** (or **Add to Home screen**). Fileship appears in the app drawer and on the home screen.

**iPhone and iPad (Safari):** open your Fileship, tap the share button and choose **Add to Home Screen**.

**Desktop (Chrome, Edge):** click the install icon in the address bar, or choose **Install Fileship** from the browser menu.

## Share to Fileship (Android)

1. In any app, choose **Share** and pick **Fileship** in the share sheet.
2. Fileship opens on the **Share to Fileship** page. If you are logged out, you log in first; what you shared is kept on the phone until you are back.
3. Choose what happens to each item:
    - **Files** are uploaded to your account with the same options as the upload sheet: folder, file name style, expiry, compression, view limit and password.
    - **Links** can be shortened or saved as a note.
    - **Text** is saved as a note. You can change its title and content and make it public.
4. Tap **Save**. The page lists the links to everything you saved, each with a copy button, and **Copy all** copies them together.

Items that fail stay on the page with the error, so you can tap **Save** again. **Discard** removes everything you shared without saving it. Shared items are kept only in the app's storage on your phone and are removed as soon as they are saved or discarded; anything left behind is cleared after a day, the next time you share something.

What you can save depends on your account's permissions: uploading files, taking notes and shortening links are separate permissions.

If you installed Fileship before your server was updated to a version with sharing, Fileship may not appear in the share sheet right away. Android picks up the new app settings within a day of opening the app; removing the app and installing it again makes it appear immediately.

## What iOS does not support

Safari on iPhone and iPad does not support sharing to web apps (the Web Share Target API), so Fileship never appears in the iOS share sheet, even when it is installed on the home screen. On iOS, open Fileship and use **Upload** instead, or create links and notes from their pages.

Desktop browsers also do not offer installed web apps in their share menus, except on ChromeOS.
