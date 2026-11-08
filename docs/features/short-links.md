---
title: Short links
description: Shorten links with Fileship, with a custom code, a password, a click limit or an expiry, and see how often they are clicked.
---

# Short links

The **Links** page lists your short links. It needs the **Shorten links** permission.

**Shorten link** creates one:

| Field          | What it does                                                                                                           |
| -------------- | ---------------------------------------------------------------------------------------------------------------------- |
| **URL**        | The target. Only `http://` and `https://` addresses, up to 2048 characters.                                            |
| **Short link** | The code after `/s/`. Leave **Random** for a generated code, or type up to 32 letters, numbers, dashes or underscores. |
| **Password**   | Visitors must enter it before they are sent on.                                                                        |
| **Max clicks** | The link is deleted at the click that reaches it. 0 means unlimited.                                                   |
| **Expiration** | The link stops working after this time.                                                                                |

The short link (`/s/<code>`) is copied to the clipboard when it is created. Each link shows its **Clicks**; your own clicks while logged in are not counted. **Edit** changes everything above and applies right away.

Short links redirect straight to the target, so apps like Discord show the target's preview. Password protected links show a page that asks for the password first.

Links use one of your [custom domains](../proxy/custom-domains.md) when you have some.

You can also shorten links from [ShareX](../uploaders/sharex.md#url-shortener), the [macOS](../uploaders/macos.md) and [Linux](../uploaders/linux.md) scripts (`--link`), [Fileship for Windows](../desktop.md), the [browser extension](../browser-extension.md), the [Android share sheet](../mobile.md) and the [API](../api/uploads.md#shorten-a-link).
