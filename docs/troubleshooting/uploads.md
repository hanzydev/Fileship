---
title: Upload problems
description: Fix failed uploads in Fileship - 413 errors from a reverse proxy, "Storage limit reached", "Invalid upload chunk", "The request is too large" and name conflicts.
---

# Upload problems

## Upload failed with 413 (Request Entity Too Large)

The error page or ShareX's message comes from your **reverse proxy**, not from Fileship: the request body is larger than the proxy allows.

- Dashboard, Windows app and extension uploads fail with 413 when the proxy's limit is smaller than **Chunk size (MB)** (25 MB by default).
- ShareX, the scripts and `curl` fail for files larger than the limit, because they send the whole file in one request.

Raise or remove the limit: `client_max_body_size 0;` in Nginx (default 1 MB), `LimitRequestBody 0` in Apache, `maxAllowedContentLength` in IIS (default about 28.6 MB). Caddy and Traefik have no limit by default. Cloudflare's proxy and tunnels cap requests at 100 MB on the Free plan. See [Upload size](../proxy/overview.md#upload-size).

## Storage limit reached

Your account has a storage limit, and the upload would go past it. Files in the trash count too: empty the trash or delete large files (**Settings → Storage** lists the largest), or ask an admin for more space on the **Users** page. The dashboard warns before the upload starts (**Not enough space**).

In a shared folder, files count toward the folder **owner's** limit, so the owner needs the space.

## The request is too large

Fileship refuses API requests that are not uploads when they are larger than 1 MB (21 MB for OCR, and the avatar size limit for profile updates). You see it when:

- an avatar is larger than **Avatar size limit (MB)**
- a script sends a file to an endpoint that is not an upload endpoint

Uploads to `/api/files` and the inbox are never limited by size, only by the storage limit.

## The request must include its length

A client sent a request with `Transfer-Encoding: chunked` and no `Content-Length` to an endpoint that needs one. Send the body with its length. Upload endpoints accept chunked bodies.

## Invalid upload chunk

A piece of a [chunked upload](../api/uploads.md#chunked-uploads) does not fit the others: different sizes, a wrong `chunkOffset`, a different `totalChunks`, or a last piece larger than the rest. This happens with custom clients; the dashboard and the apps always send matching pieces. A proxy that changes request bodies can cause it too.

**Upload not found** means the upload session expired (no piece for 6 hours) or was already finished; start the upload again.

## A file with that name already exists

You chose **Original** as the file name, and another file (anyone's, on the whole instance) already has that link name. **A file with that name is in the trash** means the other file is in a trash. Choose **Random** or **UUID**, rename your file, or delete the other one for good.

**This file name is reserved** and **File name cannot start with a dot** are about the link name too.

## Uploads stop halfway or time out

- A proxy timeout shorter than the time a chunk takes: raise `proxy_read_timeout` / `ProxyTimeout` (the examples use 300 seconds).
- A slow storage (SFTP, FTP or a distant S3 region) makes each chunk slower; FTP also takes one chunk at a time.
- Big images with **Remove EXIF data** on are assembled and re-encoded before they are saved, which takes longer.

## Uploads work in the dashboard but not in ShareX

ShareX sends the whole file at once; see [413](#upload-failed-with-413-request-entity-too-large). Also check that the config was downloaded from the public address (its `RequestURL`), and that the token was not regenerated since.

## Not enough space on the server

Uploads to the data directory fail when the disk is full, and chunks are buffered in `data/temp` for images that need processing. Check free space with `df -h`.
