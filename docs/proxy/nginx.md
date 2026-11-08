---
title: Nginx
description: Serve Fileship on your domain with Nginx, over HTTPS with Certbot or plain HTTP, for Docker and non-Docker installs.
---

# Nginx

[Nginx](https://nginx.org/) needs a few lines more than Caddy: WebSocket upgrades for `/socket.io/`, the forwarded headers and a larger body limit (its default is 1 MB). Read the [overview](overview.md) first if you have not.

## Install Nginx

On Debian and Ubuntu:

```sh
sudo apt install nginx
```

Site configurations go in `/etc/nginx/sites-available/` and are enabled with a link in `/etc/nginx/sites-enabled/`. Other distributions use `/etc/nginx/conf.d/*.conf`.

## HTTP

Create `/etc/nginx/sites-available/fileship`:

```nginx
map $http_upgrade $connection_upgrade {
    default upgrade;
    '' close;
}

server {
    listen 80;
    listen [::]:80;
    server_name files.example.com;

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
        proxy_read_timeout 300s;
        proxy_send_timeout 300s;
    }
}
```

Enable it and reload:

```sh
sudo ln -s /etc/nginx/sites-available/fileship /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

`127.0.0.1:3000` is the built-in proxy with [Docker](../installation.md) and the app itself [without Docker](../install/without-docker.md); the configuration is the same.

What the lines do:

- `client_max_body_size 0` turns off Nginx's body limit, so ShareX and scripts can upload large files in one request. Set a size such as `2G` to cap it, but keep it above the chunk size (25 MB by default).
- `proxy_request_buffering off` streams uploads to Fileship instead of writing them to a temporary file first.
- `Upgrade` and `Connection` let socket.io switch to WebSockets. The `map` sends `Connection: close` for normal requests.
- `$proxy_add_x_forwarded_for` appends the client address to `X-Forwarded-For`, which is how Fileship gets the [client IP](overview.md#client-ip).
- `X-Fileship-Client-IP $remote_addr` overwrites a header Fileship trusts from proxies. Keep it, or a visitor could send their own and pretend to have another IP address.

## HTTPS with Certbot

Start with the HTTP configuration above, point your domain's DNS at the server and open ports 80 and 443. Then let [Certbot](https://certbot.eff.org/) get a certificate and change the configuration for you:

```sh
sudo apt install certbot python3-certbot-nginx
sudo certbot --nginx -d files.example.com
```

Certbot adds the `listen 443 ssl` lines, the certificate paths and a redirect from HTTP to HTTPS, and renews the certificate automatically. If you prefer to write the HTTPS server yourself, this is the full result:

```nginx
map $http_upgrade $connection_upgrade {
    default upgrade;
    '' close;
}

server {
    listen 80;
    listen [::]:80;
    server_name files.example.com;

    return 301 https://$host$request_uri;
}

server {
    listen 443 ssl;
    listen [::]:443 ssl;
    http2 on;
    server_name files.example.com;

    ssl_certificate /etc/letsencrypt/live/files.example.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/files.example.com/privkey.pem;

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
        proxy_read_timeout 300s;
        proxy_send_timeout 300s;
    }
}
```

`http2 on;` needs Nginx 1.25.1 or newer; on older versions write `listen 443 ssl http2;` instead.

## Behind another TLS terminator

When a load balancer or CDN in front of Nginx terminates HTTPS and talks to Nginx over HTTP, `$scheme` is `http`. Pass on the protocol it sends instead: add this `map` next to the other one and change the `X-Forwarded-Proto` line:

```nginx
map $http_x_forwarded_proto $forwarded_proto {
    default $http_x_forwarded_proto;
    '' $scheme;
}
```

```nginx
proxy_set_header X-Forwarded-Proto $forwarded_proto;
```

Only do this when Nginx is reachable from that terminator alone, because clients could otherwise send their own header. Setting **Link protocol** to **Always HTTPS** is the alternative (see [Link protocol](overview.md#link-protocol)).

## Nginx in Docker

Run the official image next to the Fileship stack and reach the host's port 3000 through `host.docker.internal`:

```yaml
services:
    nginx:
        image: nginx:alpine
        restart: unless-stopped
        ports:
            - '80:80'
        extra_hosts:
            - 'host.docker.internal:host-gateway'
        volumes:
            - ./fileship.conf:/etc/nginx/conf.d/default.conf:ro
```

Use one of the configurations above as `fileship.conf`, with `proxy_pass http://host.docker.internal:3000;`.

## Nginx Proxy Manager

In [Nginx Proxy Manager](https://nginxproxymanager.com/), add a proxy host with the scheme `http`, the forward host and port of Fileship (the host's address and `3000`) and **Websockets Support** on. In the **Advanced** tab add:

```nginx
client_max_body_size 0;
proxy_request_buffering off;
```

Nginx Proxy Manager sets `Host`, `X-Forwarded-For` and `X-Forwarded-Proto` itself. This setup was not tested.

## Check it

Follow [After the proxy works](overview.md#after-the-proxy-works).

> [!NOTE]
> Not tested yet: the configurations on this page follow Nginx's documentation, and runs in Docker against a Fileship instance (login, an upload larger than the chunk size, a single-request upload, live updates and the client IP) are still to be done.
