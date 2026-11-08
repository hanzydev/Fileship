---
title: Traefik
description: Serve Fileship on your domain with Traefik v3, with Let's Encrypt certificates or plain HTTP, for Docker and non-Docker installs.
---

# Traefik

[Traefik](https://traefik.io/traefik/) passes WebSockets, the `Host` header and the `X-Forwarded-*` headers by default and has no body limit, so Fileship only needs a router and a service. These examples use Traefik v3 with a file provider, which works the same whether Fileship runs in Docker or not. Read the [overview](overview.md) first if you have not.

## Traefik in Docker

`docker-compose.yml` for Traefik, in its own folder:

```yaml
services:
    traefik:
        image: traefik:v3
        restart: unless-stopped
        command:
            - --providers.file.filename=/etc/traefik/fileship.yml
            - --entrypoints.web.address=:80
            - --entrypoints.websecure.address=:443
            - --entrypoints.web.http.redirections.entrypoint.to=websecure
            - --entrypoints.web.http.redirections.entrypoint.scheme=https
            - --certificatesresolvers.letsencrypt.acme.email=you@example.com
            - --certificatesresolvers.letsencrypt.acme.storage=/letsencrypt/acme.json
            - --certificatesresolvers.letsencrypt.acme.httpchallenge.entrypoint=web
        ports:
            - '80:80'
            - '443:443'
        extra_hosts:
            - 'host.docker.internal:host-gateway'
        volumes:
            - ./fileship.yml:/etc/traefik/fileship.yml:ro
            - ./letsencrypt:/letsencrypt
```

`fileship.yml` next to it:

```yaml
http:
    routers:
        fileship:
            rule: Host(`files.example.com`)
            entryPoints: [websecure]
            service: fileship
            middlewares: [fileship-headers]
            tls:
                certResolver: letsencrypt

    middlewares:
        fileship-headers:
            headers:
                customRequestHeaders:
                    X-Fileship-Client-IP: ''

    services:
        fileship:
            loadBalancer:
                servers:
                    - url: http://host.docker.internal:3000
```

Point your domain's DNS at the server, open ports 80 and 443, and start it with `docker compose up -d`. Traefik gets a Let's Encrypt certificate on the first request and redirects HTTP to HTTPS.

The `fileship-headers` middleware removes `X-Fileship-Client-IP` from incoming requests. Keep it: Fileship trusts that header from a proxy, so without it a visitor could send their own and pretend to have another IP address. Fileship then uses the `X-Forwarded-For` entry Traefik adds.

`host.docker.internal:3000` is the host's port 3000: the built-in proxy with [Docker](../installation.md), or the app [without Docker](../install/without-docker.md) when it listens on an address the container can reach (`HOST=0.0.0.0`, with port 3000 closed in the firewall). Requests arrive from the Docker network's gateway, a private address, so the [client IP](overview.md#client-ip) comes through.

## HTTP only

Drop the certificate resolver, the redirect and the `websecure` entry point:

```yaml
command:
    - --providers.file.filename=/etc/traefik/fileship.yml
    - --entrypoints.web.address=:80
```

```yaml
http:
    routers:
        fileship:
            rule: Host(`files.example.com`)
            entryPoints: [web]
            service: fileship
            middlewares: [fileship-headers]

    middlewares:
        fileship-headers:
            headers:
                customRequestHeaders:
                    X-Fileship-Client-IP: ''

    services:
        fileship:
            loadBalancer:
                servers:
                    - url: http://host.docker.internal:3000
```

## Traefik without Docker

Running the `traefik` binary on the same machine, put the same options in a static configuration file and point the service at `http://127.0.0.1:3000`:

```yaml
# /etc/traefik/traefik.yml
providers:
    file:
        filename: /etc/traefik/fileship.yml

entryPoints:
    web:
        address: ':80'
        http:
            redirections:
                entryPoint:
                    to: websecure
                    scheme: https
    websecure:
        address: ':443'

certificatesResolvers:
    letsencrypt:
        acme:
            email: you@example.com
            storage: /etc/traefik/acme.json
            httpChallenge:
                entryPoint: web
```

## Labels instead of a file

If Traefik already watches Docker labels, you can route to the built-in proxy by joining it to Traefik's network. That means editing the `proxy` service in Fileship's `docker-compose.yml` (labels and an external network), which you then have to keep when you `git pull`. The file provider above avoids that.

## Behind another TLS terminator

Traefik replaces incoming `X-Forwarded-*` headers unless it trusts the sender. When a load balancer or CDN terminates HTTPS in front of Traefik, list its addresses on the entry point:

```yaml
- --entrypoints.web.forwardedheaders.trustedips=10.0.0.0/8,172.16.0.0/12,192.168.0.0/16
```

## Check it

Follow [After the proxy works](overview.md#after-the-proxy-works).

> [!NOTE]
> Not tested yet: the configurations on this page follow Traefik's documentation, and runs in Docker against a Fileship instance (login, an upload larger than the chunk size, a single-request upload, live updates and the client IP) are still to be done.
