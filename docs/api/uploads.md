---
title: Upload API
description: Upload files and shorten links from your own scripts and apps with Fileship's upload token - authentication, fields, chunked uploads and responses.
---

# Upload API

Everything the uploaders on these pages do goes through two endpoints: `POST /api/files` to upload a file and `POST /api/links` to shorten a link. You can call them from any tool that sends HTTP requests.

## Upload token

Open **Settings → Integrations** and copy the **Upload token** (it starts with `fs_`). Send it in the `Authorization` header:

```http
Authorization: Bearer fs_...
```

The upload token only works on `POST /api/files` and `POST /api/links`. It cannot list, read or delete anything, so it is safe to put in a script, but keep it private. **Regenerate** makes a new one and stops the old one; configs and scripts that use it stop working.

Uploading needs the **Upload files** permission and shortening the **Shorten links** permission.

## Upload a file

`POST /api/files` with a `multipart/form-data` body. The file goes in the `file` field; everything else is optional:

| Field          | Type                           | Description                                                                                    |
| -------------- | ------------------------------ | ---------------------------------------------------------------------------------------------- |
| `file`         | file                           | The file. Required.                                                                            |
| `fileNameType` | `Random`, `UUID` or `Original` | The link name. Default: the original name.                                                     |
| `expiration`   | number                         | Milliseconds until the file is deleted, for example `86400000` for a day. `0` or empty: never. |
| `maxViews`     | number                         | Delete after this many views. `0`: unlimited.                                                  |
| `password`     | string                         | Visitors must enter it. Up to 48 characters.                                                   |
| `compression`  | 0 to 100                       | Re-encode images as JPEG at this compression. `0`: keep the original.                          |
| `folderId`     | string                         | Put the file in this folder (yours, or a shared folder where you are an editor).               |

With `curl`:

```sh
curl -H "Authorization: Bearer $FILESHIP_TOKEN" \
  -F "file=@screenshot.png" \
  -F "fileNameType=Random" \
  https://files.example.com/api/files
```

The response is the new file as JSON. The fields you usually need:

| Field          | Description                                                                                                |
| -------------- | ---------------------------------------------------------------------------------------------------------- |
| `url`          | The link to share: the file page (`/view/…`) when embeds are on or for text files, else the file (`/u/…`). |
| `directUrl`    | The file itself, `/u/<fileName>`.                                                                          |
| `embedUrl`     | The file page, `/view/<fileName>`.                                                                         |
| `thumbnailUrl` | The thumbnail, for images, videos and PDFs.                                                                |
| `id`           | The file's id.                                                                                             |
| `fileName`     | The link name.                                                                                             |
| `name`         | The display name (the original name).                                                                      |
| `size`         | `{ "raw": "<bytes>", "formatted": "1.2 MB" }`.                                                             |

Links use one of your [custom domains](../proxy/custom-domains.md) when you have some, and follow the [link protocol](../proxy/overview.md#link-protocol).

A request carries the whole file, however large. Make sure your [reverse proxy](../proxy/overview.md#upload-size) allows such bodies, or use chunks.

## Chunked uploads

For large files, send the file in pieces, the way the dashboard, the Windows app and the browser extension do. Each piece is one `POST /api/files` request with the same fields as above plus:

| Field          | Description                                                                                     |
| -------------- | ----------------------------------------------------------------------------------------------- |
| `uploadId`     | An id you choose for this upload, 1 to 64 letters, digits, `_` or `-`. The same in every piece. |
| `currentChunk` | The number of this piece, starting at 1.                                                        |
| `totalChunks`  | How many pieces there are, at most 10,000.                                                      |
| `chunkOffset`  | Where this piece starts in the file, in bytes: `(currentChunk - 1) × chunk size`.               |

Rules:

- Every piece except the last has the same size, and the last is not larger. Use the server's chunk size: `GET /api/settings/public` returns `fileChunkSize` in MB. Pieces larger than it are refused.
- Pieces can arrive in any order and in parallel. `uploadConcurrency` in the same response says how many to send at once (1 for FTP storage, otherwise 4).
- Send the same options (`fileNameType`, `expiration`, …) with every piece; the name is chosen with the first one.
- Pieces before the last answer with an empty response. The request that completes the file returns the file as above.
- A piece that does not fit the layout fails with **Invalid upload chunk**. An upload with no new piece for 6 hours is discarded.
- A piece can be sent again (after a network error, or to resume a paused upload); it replaces the earlier copy. A piece whose request was cut off before its body arrived is not stored. Only the last piece must not be sent again once its whole body went out, because the server may already have completed the file.
- To cancel, send `DELETE /api/files/uploads/<uploadId>` with a session (not an upload token). It drops the pieces received so far and answers 204, also when there is nothing to drop.

## Shorten a link

`POST /api/links` with a JSON body:

| Field        | Type   | Description                                                                   |
| ------------ | ------ | ----------------------------------------------------------------------------- |
| `url`        | string | The target, `http://` or `https://`, up to 2048 characters. Required.         |
| `code`       | string | The code after `/s/`, up to 32 letters, numbers, `-` or `_`. Default: random. |
| `password`   | string | Visitors must enter it. Up to 48 characters.                                  |
| `maxClicks`  | number | Delete the link at this many clicks. `0`: unlimited.                          |
| `expiration` | number | Milliseconds until the link stops working.                                    |

```sh
curl -H "Authorization: Bearer $FILESHIP_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{"url":"https://example.com/a/long/link"}' \
  https://files.example.com/api/links
```

The response is the link as JSON, with the short address in `shortUrl`.

## Errors

Errors come back as JSON with a `message` in the language of the request's `Accept-Language` header, and a status code:

| Status | Meaning                                                                                                                                    |
| ------ | ------------------------------------------------------------------------------------------------------------------------------------------ |
| 400    | A field is invalid (**Some fields are invalid**), the file is missing, or **Storage limit reached** (then `data.quotaExceeded` is `true`). |
| 401    | The token is missing or wrong (**You must be logged in to perform this action**).                                                          |
| 403    | The account lacks the permission (**You do not have permission to perform this action**).                                                  |
| 409    | The link name or code is taken (**A file with that name already exists**, **That short link is already taken**).                           |
| 413    | Your proxy refused the body; see [Upload problems](../troubleshooting/uploads.md).                                                         |

Don't compare the `message` text in your code; it is translated. Check the status and the `data` flags. See the [error index](../troubleshooting/index.md) for every message.

## Other uploaders

Ready-made configs: [ShareX](../uploaders/sharex.md), [macOS](../uploaders/macos.md), [Linux](../uploaders/linux.md), [Flameshot](../uploaders/flameshot.md), [iOS Shortcuts](../uploaders/ios-shortcuts.md), [Android](../uploaders/android.md) and [curl](../uploaders/curl.md).
