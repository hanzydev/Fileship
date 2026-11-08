---
title: curl
description: Upload files and shorten links on Fileship from the command line with curl, on Linux, macOS and Windows.
---

# curl

Any command line can upload to Fileship with `curl` and your upload token (**Settings → Integrations → Upload token**). See the [Upload API](../api/uploads.md) for every field.

## Linux and macOS

```sh
export FILESHIP_URL=https://files.example.com
export FILESHIP_TOKEN=fs_...

# upload a file and print the link
curl -fsS -H "Authorization: Bearer $FILESHIP_TOKEN" \
  -F "file=@photo.jpg" \
  "$FILESHIP_URL/api/files" | jq -r .url

# with options: random name, expires in a day, password
curl -fsS -H "Authorization: Bearer $FILESHIP_TOKEN" \
  -F "file=@photo.jpg" -F fileNameType=Random -F expiration=86400000 -F password=secret \
  "$FILESHIP_URL/api/files" | jq -r .url

# shorten a link
curl -fsS -H "Authorization: Bearer $FILESHIP_TOKEN" -H "Content-Type: application/json" \
  -d '{"url":"https://example.com/a/long/link"}' \
  "$FILESHIP_URL/api/links" | jq -r .shortUrl
```

Without `jq`, drop `| jq -r .url` to see the whole JSON. `-f` makes curl fail on errors; leave it out to see Fileship's error message.

## Windows

Windows 10 and 11 include `curl.exe`. In PowerShell, call it as `curl.exe` (plain `curl` is an alias for `Invoke-WebRequest` in Windows PowerShell):

```powershell
$env:FILESHIP_URL = "https://files.example.com"
$env:FILESHIP_TOKEN = "fs_..."

curl.exe -fsS -H "Authorization: Bearer $env:FILESHIP_TOKEN" -F "file=@photo.jpg" "$env:FILESHIP_URL/api/files" |
  ConvertFrom-Json | Select-Object -ExpandProperty url
```

## Large files

A single request carries the whole file. Through a [reverse proxy](../proxy/overview.md#upload-size), make sure it accepts bodies that large, or use [chunked uploads](../api/uploads.md#chunked-uploads).
