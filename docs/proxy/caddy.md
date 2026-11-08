---
title: Caddy
description: Serve Fileship on your domain with Caddy, with automatic HTTPS or plain HTTP, for Docker and non-Docker installs.
---

# Caddy

[Caddy](https://caddyserver.com/) gets and renews HTTPS certificates by itself and passes WebSockets, the `Host` header and the `X-Forwarded-*` headers without extra settings, so it is the shortest setup. Read the [overview](overview.md) first if you have not.

This page is about a Caddy you run yourself in front of Fileship. The Docker stack also contains a Caddy (the `proxy` service with `caddy.json`); leave that one as it is.

## Install Caddy

Follow [Caddy's install guide](https://caddyserver.com/docs/install) for your system. On Debian and Ubuntu it installs a `caddy` service that reads `/etc/caddy/Caddyfile`. On Windows, download `caddy.exe` and see [Windows](#windows).

## HTTPS (automatic)

Point your domain's DNS `A` (and `AAAA`) record at the server and open ports 80 and 443. Then put this in `/etc/caddy/Caddyfile`:

```caddy
files.example.com {
    reverse_proxy 127.0.0.1:3000 {
        header_up X-Fileship-Client-IP {client_ip}
    }
}
```

Reload Caddy:

```sh
sudo systemctl reload caddy
```

Caddy gets a Let's Encrypt certificate on the first request and redirects HTTP to HTTPS. The same block works for both install types: with [Docker](../installation.md) port 3000 is the built-in proxy, [without Docker](../install/without-docker.md) it is the app.

The `header_up` line overwrites `X-Fileship-Client-IP` with the real client address. Keep it: Fileship trusts that header from a proxy, and without the line a visitor could send their own and pretend to have another IP address (see [Client IP](overview.md#client-ip)).

Caddy has no request body limit by default, so uploads of any size pass. To cap single-request uploads, add `request_body { max_size 2GB }` inside the site block.

## HTTP only

Use this for a local network, or when another service in front of Caddy handles HTTPS. Prefix the address with `http://` so Caddy does not try to get a certificate:

```caddy
http://files.example.com {
    reverse_proxy 127.0.0.1:3000 {
        header_up X-Fileship-Client-IP {client_ip}
    }
}
```

Use `:80` instead of a domain to answer on every address.

### Behind another TLS terminator

When a load balancer or CDN terminates HTTPS and talks to Caddy over HTTP, Caddy only passes its `X-Forwarded-Proto: https` on when it trusts it. Tell Caddy which addresses it is (here a private network):

```caddy
{
    servers {
        trusted_proxies static private_ranges
    }
}

http://files.example.com {
    reverse_proxy 127.0.0.1:3000 {
        header_up X-Fileship-Client-IP {client_ip}
    }
}
```

Without this, links come out as `http://`. You can also set **Link protocol** to **Always HTTPS** (see [Link protocol](overview.md#link-protocol)).

## Caddy in Docker

To run Caddy as a container next to the Fileship stack, it cannot reach `127.0.0.1:3000` of the host. Use the host's address through `host.docker.internal`:

```yaml
services:
    caddy:
        image: caddy:2-alpine
        restart: unless-stopped
        ports:
            - '80:80'
            - '443:443'
            - '443:443/udp'
        extra_hosts:
            - 'host.docker.internal:host-gateway'
        volumes:
            - ./Caddyfile:/etc/caddy/Caddyfile:ro
            - caddy_data:/data

volumes:
    caddy_data:
```

```caddy
files.example.com {
    reverse_proxy host.docker.internal:3000 {
        header_up X-Fileship-Client-IP {client_ip}
    }
}
```

Requests then arrive at port 3000 from the Docker network's gateway, a private address, so the client IP still comes through. Keep port 3000 published on the host (the default) for this to work.

## Windows

1. Download `caddy.exe` from [caddyserver.com/download](https://caddyserver.com/download) and put it in a folder such as `C:\Caddy`.
2. Create `C:\Caddy\Caddyfile` with the HTTPS block above.
3. Allow ports 80 and 443 in the firewall (PowerShell as administrator):

    ```powershell
    New-NetFirewallRule -DisplayName "Caddy HTTP/HTTPS" -Direction Inbound -Protocol TCP -LocalPort 80,443 -Action Allow
    ```

4. Test it from `C:\Caddy` with `.\caddy.exe run`, then run it as a service with [NSSM](https://nssm.cc/) or the Windows service wrapper described in [Caddy's docs](https://caddyserver.com/docs/running#windows-service).

See [Windows](../install/windows.md) for the whole Windows setup.

## Check it

Follow [After the proxy works](overview.md#after-the-proxy-works): set the **Dashboard address**, log in, upload a large file and check the IP in **Settings → Sessions**.

> [!NOTE]
> Not tested yet: the configurations on this page follow Caddy's documentation, and runs in Docker against a Fileship instance (login, an upload larger than the chunk size, a single-request upload, live updates and the client IP) are still to be done.
