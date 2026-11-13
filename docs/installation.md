---
title: Install with Docker
description: Install Fileship with Docker Compose in three commands, log in for the first time and put it behind your domain.
---

# Install with Docker

Docker Compose is the recommended way to run Fileship. The stack updates itself without downtime, upgrades PostgreSQL by itself and comes with a small proxy that keeps Fileship reachable during updates. To run it without Docker, see [Install without Docker](install/without-docker.md); for a Windows server, see [Windows](install/windows.md).

## Requirements

A server with [Docker](https://www.docker.com/) and [Docker Compose](https://docs.docker.com/compose/) v2, x86-64 or ARM64, and at least 2 cores, 2 GB of RAM and 4 GB of disk with AI off, or 4 to 8 GB of RAM with AI on. [Requirements](install/requirements.md) has the table for each AI preset and the supported browsers.

PostgreSQL 18 is included in the Compose stack and is kept up to date by the updater agent, including new major versions (see [Updating](updating.md#postgresql)).

## Install

**1. Clone the repository**

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
```

**2. Generate the updater secret**

Fileship ships with a small `updater-agent` service that performs updates. Generate the secret it shares with the app:

```sh
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
```

**3. Start Fileship**

```sh
docker compose up -d
```

Fileship is now available at `http://localhost:3000`, or `http://<server address>:3000` from another machine. The first start downloads the images and creates the database, which takes a minute or two.

## Default credentials

- Username: `admin`
- Password: `password`

> [!IMPORTANT]
> Change the password right after your first login (**Settings → Security**).

[First steps](install/first-steps.md) walks through the rest: the dashboard address, users, and connecting ShareX or the apps.

## What runs

`docker-compose.yml` starts these services:

| Service          | What it does                                                                                          |
| ---------------- | ----------------------------------------------------------------------------------------------------- |
| `postgres`       | PostgreSQL 18 (`ghcr.io/hanzydev/fileship-postgres`), data in the `pg_data` volume                    |
| `proxy`          | Caddy on port 3000 (`caddy.json`), sends each request to the healthy Fileship container               |
| `fileship-blue`  | Fileship itself (`ghcr.io/hanzydev/fileship`)                                                         |
| `fileship-green` | The second Fileship container, started only during an update                                          |
| `fileship-ml`    | The [AI service](ai.md#the-ai-service) (`ghcr.io/hanzydev/fileship-ml`), models in `data/models`      |
| `updater-agent`  | Performs [updates](updating.md) through the Docker socket (`ghcr.io/hanzydev/fileship-updater-agent`) |

Your files live in the `data` folder next to `docker-compose.yml` (mounted at `/fileship/data`), and the database in the `pg_data` volume. Back up both, and keep `data/secret.key`: storage credentials, upload tokens, webhook URLs and email and single sign-on secrets are encrypted with it. See [Backing up the database](backups/database.md).

To change a setting through the environment, see [Configuration](configuration.md#environment-only-settings).

## Browser compatibility

Fileship targets current versions of Chrome, Edge, Firefox, Brave and Zen Browser. Safari is supported through compatibility fallbacks. See [Requirements](install/requirements.md#browsers).

## Web server configuration (optional)

To serve Fileship on your own domain, put a reverse proxy in front of it that sends requests to `127.0.0.1:3000`, the built-in proxy. [Reverse proxy overview](proxy/overview.md) explains what the proxy must pass on, with full examples for [Caddy](proxy/caddy.md), [Nginx](proxy/nginx.md), [Apache](proxy/apache.md), [Traefik](proxy/traefik.md), [IIS](proxy/iis.md) and [Cloudflare Tunnel](proxy/cloudflare-tunnel.md).

A minimal [Nginx](https://nginx.org/) example:

```nginx
map $http_upgrade $connection_upgrade {
    default upgrade;
    '' close;
}

server {
    listen 80;
    server_name <domain>;

    client_max_body_size 0;

    location / {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;

        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Fileship-Client-IP $remote_addr;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection $connection_upgrade;

        proxy_request_buffering off;
    }
}
```

WebSocket upgrades and the `X-Forwarded-*` headers pass through, so Fileship still sees the real client IP. The built-in proxy has no upload size limit of its own, so `client_max_body_size` is the only one that applies. The dashboard uploads in chunks (25 MB by default), but ShareX and the upload scripts send each file in one request, which is why the example removes the limit. Add HTTPS with [Certbot](proxy/nginx.md#https-with-certbot).

## Troubleshooting

If a container keeps restarting or the page does not load, see [Installation and startup problems](troubleshooting/install-and-startup.md).
