---
title: Reverse proxy overview
description: What a reverse proxy in front of Fileship has to pass on, how Fileship finds the client IP, and which page to follow for your web server.
---

# Reverse proxy overview

A reverse proxy puts Fileship on your own domain, usually with HTTPS. It receives the requests on ports 80 and 443 and passes them to Fileship on port 3000.

Pick the page for your web server:

| Web server                                | HTTPS certificates                       |
| ----------------------------------------- | ---------------------------------------- |
| [Caddy](caddy.md)                         | automatic                                |
| [Nginx](nginx.md)                         | Certbot                                  |
| [Apache](apache.md)                       | Certbot                                  |
| [Traefik](traefik.md)                     | automatic (ACME)                         |
| [IIS](iis.md) (Windows)                   | win-acme or your own certificate         |
| [Cloudflare Tunnel](cloudflare-tunnel.md) | Cloudflare, no open ports on your server |

[SSL certificates](ssl.md) explains the options for certificates, and [Custom domains](custom-domains.md) how to add extra domains for your links.

## Where the proxy sends requests

| How you installed Fileship                     | Send requests to                                       |
| ---------------------------------------------- | ------------------------------------------------------ |
| [Docker](../installation.md)                   | `127.0.0.1:3000`, the Caddy proxy built into the stack |
| [Without Docker](../install/without-docker.md) | `127.0.0.1:3000`, the app itself (`HOST` and `PORT`)   |

With Docker, the stack already runs a small Caddy proxy (the `proxy` service) that listens on port 3000 and sends each request to the healthy app container during updates. Your own proxy goes in front of it; do not point it at the `fileship-blue` or `fileship-green` containers, or updates stop being seamless.

> [!TIP]
> When your proxy runs on the same machine, make port 3000 reachable only from there. With Docker, change the `proxy` port mapping in `docker-compose.yml` to `'127.0.0.1:3000:3000'` and run `docker compose up -d`. Without Docker, set `HOST=127.0.0.1`. Docker publishes ports past most host firewalls (such as `ufw`), so a firewall rule alone is not enough.

## What the proxy must pass on

Every example in these pages does the following. Check them when you write your own configuration.

| Requirement                                        | Why                                                                                                                                                                                                                                                       |
| -------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Keep the `Host` header (or set `X-Forwarded-Host`) | Fileship builds the links it returns (`/u/…`, `/s/…`, `/view/…`) from the address the request came to.                                                                                                                                                    |
| Set `X-Forwarded-Proto`                            | Links use `https://` when the request was HTTPS, and the login cookie is marked `Secure`. Without it, links come out as `http://` (see [Link protocol](#link-protocol)).                                                                                  |
| Append the client to `X-Forwarded-For`             | Fileship logs, rate limits and sessions use the client IP. See [Client IP](#client-ip).                                                                                                                                                                   |
| Pass WebSocket upgrades on `/socket.io/`           | Lists, uploads and settings update live over socket.io. Without upgrades, socket.io falls back to long polling, which works but is slower; if `/socket.io/` is blocked, nothing updates live ([Live updates don't work](../troubleshooting/realtime.md)). |
| Allow large request bodies                         | See [Upload size](#upload-size).                                                                                                                                                                                                                          |
| Keep long requests open                            | Large chunks, archives and backup restores can take minutes on a slow disk or storage.                                                                                                                                                                    |

### Upload size

The dashboard, the Windows app and the browser extension upload in chunks of **Chunk size (MB)** (25 MB by default, see [Instance settings](../configuration/instance-settings.md#uploads)), so for them the proxy's body limit only needs to be a little larger than the chunk size.

ShareX, the macOS and Linux scripts and `curl` send the whole file in one request. To upload large files with them, the proxy must accept a body as large as the largest file, or have no limit. The examples set no limit (`0`) or a large one; lower it if you want a cap.

Fileship itself has no total upload size limit. It refuses API requests that are not uploads when they are larger than 1 MB (21 MB for OCR, and the avatar size for profile updates).

### Client IP

Fileship takes the client IP from the request only when the connection comes from a private or loopback address (`10.0.0.0/8`, `172.16.0.0/12`, `192.168.0.0/16`, `127.0.0.0/8`, `::1`, `fc00::/7`), which is where a proxy on the same machine or network connects from. In that case it reads, in this order:

1. `X-Fileship-Client-IP`, which the built-in Caddy proxy sets
2. the **last** entry of `X-Forwarded-For`

Connections from a public address always use that address, so nobody on the internet can pretend to be another IP. This is why your proxy must **append** the client address to `X-Forwarded-For` (every example here does), and why the proxy should run on the same machine or a private network.

> [!WARNING]
> Fileship trusts `X-Fileship-Client-IP` from any private address, and a proxy passes request headers on unless told otherwise. Your proxy must **overwrite or remove** `X-Fileship-Client-IP`, or a visitor can send their own and pretend to have any IP address, which also gets around the login rate limit. Every example in these pages does this. With Docker, the built-in Caddy overwrites it as well, but keep the line in your proxy anyway.

With Docker, the built-in Caddy trusts the same private ranges. It reads the client from your proxy's `X-Forwarded-For`, sets `X-Fileship-Client-IP` and passes `X-Forwarded-Proto` and `X-Forwarded-Host` through.

If every log entry and session shows the same address, such as `172.18.0.1`, see [Everyone has the same IP](../troubleshooting/login-and-security.md#everyone-has-the-same-ip).

### Link protocol

**Link protocol** in **Administration → Instance → General** sets the protocol of the links Fileship returns:

- **Same as the request** (default): `https://` when `X-Forwarded-Proto` says the request was HTTPS
- **Always HTTPS**: useful when a TLS terminator in front of your proxy does not send `X-Forwarded-Proto`
- **Always HTTP**

The `NUXT_PUBLIC_RETURN_HTTPS` variable (`auto`, `true` or `false`) sets the default.

## After the proxy works

1. Open **Administration → Instance → General** and set **Dashboard address** to the public address, for example `https://files.example.com`. Emails and single sign-on build their links from it.
2. Log in at the new address. The browser treats each address as a separate site, so you log in again there.
3. Upload a file larger than the chunk size and check that the list updates without a reload, which proves uploads and socket.io both pass through.
4. Open **Settings → Sessions** and check that your session shows your real IP address.

## Tested configurations

Each page says whether its configuration was tested. Runs of the Caddy, Nginx, Apache and Traefik examples in Docker (straight to the app and in front of the built-in proxy, over HTTP and HTTPS) are still to be done.
