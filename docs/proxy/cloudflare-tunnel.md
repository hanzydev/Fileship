---
title: Cloudflare Tunnel
description: Publish Fileship through a Cloudflare Tunnel without opening ports, and what Cloudflare's upload limits mean for chunk sizes and ShareX.
---

# Cloudflare Tunnel

A [Cloudflare Tunnel](https://developers.cloudflare.com/cloudflare-one/connections/connect-networks/) makes Fileship reachable on your domain without opening ports or having a public IP. The `cloudflared` connector runs next to Fileship and connects out to Cloudflare, which handles HTTPS.

> [!NOTE]
> This setup follows Cloudflare's documentation and has not been tested yet.

## Limits to know first

Cloudflare limits the size of one request: 100 MB on the Free and Pro plans, 200 MB on Business, 500 MB on Enterprise.

- The dashboard, the Windows app and the browser extension upload in chunks of 25 MB, well below the limit. Keep **Chunk size (MB)** under your plan's limit.
- ShareX, the scripts and `curl` send a file in **one** request, so they cannot upload files over 100 MB through the tunnel on the Free plan. Use the dashboard for those, or reach Fileship directly on your network.
- Cloudflare caches some file types by default. Fileship's files can be private or password protected, so add a cache rule that bypasses the cache for your Fileship hostname.

## Set up the tunnel

1. In the [Cloudflare dashboard](https://one.dash.cloudflare.com/), open **Networks → Tunnels → Create a tunnel**, choose **Cloudflared** and name it.
2. Copy the token from the install command it shows.
3. Add a **Public hostname**: your domain (for example `files.example.com`), service type **HTTP**, URL as below.

Run `cloudflared` with Docker, next to the Fileship stack:

```yaml
services:
    cloudflared:
        image: cloudflare/cloudflared:latest
        restart: unless-stopped
        command: tunnel --no-autoupdate run
        environment:
            TUNNEL_TOKEN: <your token>
        extra_hosts:
            - 'host.docker.internal:host-gateway'
```

| Fileship runs                                                                       | Service URL                        |
| ----------------------------------------------------------------------------------- | ---------------------------------- |
| [Docker](../installation.md)                                                        | `http://host.docker.internal:3000` |
| [Without Docker](../install/without-docker.md), `cloudflared` installed on the host | `http://127.0.0.1:3000`            |

Without Docker, install `cloudflared` from Cloudflare's packages and run `sudo cloudflared service install <token>`.

## Settings in Fileship

- Set **Dashboard address** to `https://files.example.com`.
- Cloudflare sends `X-Forwarded-Proto: https`, so **Link protocol** can stay on **Same as the request**.
- WebSockets work through tunnels by default.
- `cloudflared` connects from a private address and passes the visitor's address in `X-Forwarded-For`, so the [client IP](overview.md#client-ip) comes through.

## Cloudflare's proxy without a tunnel

If you use Cloudflare's orange-cloud proxy in front of your own web server instead, the same size limits apply. Your proxy then sees Cloudflare's addresses; restore the visitor's address with your web server's real IP module (for example Nginx's `real_ip_header CF-Connecting-IP` with Cloudflare's ranges) before it sets `X-Forwarded-For`. See [SSL certificates](ssl.md#cloudflare-origin-certificates) for certificates between Cloudflare and your server.
