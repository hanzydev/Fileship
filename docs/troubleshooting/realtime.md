---
title: Live updates don't work
description: Fix a Fileship dashboard that does not update live - lists, uploads and settings that only change after a reload - caused by socket.io not passing through a proxy.
---

# Live updates don't work

The dashboard keeps lists, storage usage, settings and other open tabs in sync over [socket.io](https://socket.io/) on `/socket.io/`. When it is blocked:

- uploads finish, but the file list only changes after a reload
- changes made in another tab or by another person never appear
- the update card does not follow an update's progress

## Check it

Open the browser's developer tools, **Network** tab, and reload. Look for requests to `/socket.io/`:

| You see                                                                         | Meaning                                                                                     |
| ------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------- |
| One request with status **101 Switching Protocols** (type websocket)            | Working.                                                                                    |
| Repeated `/socket.io/?EIO=4&transport=polling` requests with status 200, no 101 | WebSocket upgrades are blocked; socket.io falls back to long polling. It works, but slower. |
| `/socket.io/` requests with 400, 404 or 502                                     | The proxy does not pass `/socket.io/` to Fileship.                                          |

## Fix the proxy

- **Nginx:** add `proxy_http_version 1.1;` and the `Upgrade` and `Connection` headers ([Nginx](../proxy/nginx.md)).
- **Apache:** `ProxyPass / http://127.0.0.1:3000/ upgrade=websocket` with `mod_proxy_wstunnel` loaded ([Apache](../proxy/apache.md)).
- **IIS:** install the **WebSocket Protocol** feature ([IIS](../proxy/iis.md)).
- **Caddy, Traefik, Cloudflare Tunnel:** WebSockets work by default.
- **A CDN or firewall** in front of the proxy may block WebSockets; check its settings.

A proxy that closes idle connections after a short time makes the connection drop and reconnect. socket.io sends a ping every 25 seconds, so a timeout of 60 seconds or more is enough.

## Several Fileship processes

Live updates only reach browsers connected to the same Fileship process. Without Docker, run one process only (no PM2 cluster mode, no two copies behind a load balancer).

## The Windows app and Harbor

Fileship Harbor and the Windows app use the same socket. When live updates don't work in the browser, Harbor stops picking up changes from the server too.
