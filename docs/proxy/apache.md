---
title: Apache
description: Serve Fileship on your domain with the Apache HTTP Server, over HTTPS with Certbot or plain HTTP, for Docker and non-Docker installs.
---

# Apache

The [Apache HTTP Server](https://httpd.apache.org/) proxies Fileship with `mod_proxy`. It needs version 2.4.47 or newer for the WebSocket option used here. Read the [overview](overview.md) first if you have not.

## Install Apache and the modules

On Debian and Ubuntu:

```sh
sudo apt install apache2
sudo a2enmod proxy proxy_http proxy_wstunnel headers
sudo systemctl restart apache2
```

On other systems, make sure `mod_proxy`, `mod_proxy_http`, `mod_proxy_wstunnel` and `mod_headers` are loaded.

## HTTP

Create `/etc/apache2/sites-available/fileship.conf`:

```apache
<VirtualHost *:80>
    ServerName files.example.com

    ProxyPreserveHost On
    ProxyRequests Off
    ProxyTimeout 300
    LimitRequestBody 0

    RequestHeader set X-Forwarded-Proto "http"
    RequestHeader unset X-Fileship-Client-IP

    ProxyPass / http://127.0.0.1:3000/ upgrade=websocket
    ProxyPassReverse / http://127.0.0.1:3000/
</VirtualHost>
```

Enable it and reload:

```sh
sudo a2ensite fileship
sudo apachectl configtest
sudo systemctl reload apache2
```

`127.0.0.1:3000` is the built-in proxy with [Docker](../installation.md) and the app itself [without Docker](../install/without-docker.md); the configuration is the same.

What the lines do:

- `ProxyPreserveHost On` keeps the `Host` header, so links point at your domain instead of `127.0.0.1`.
- `upgrade=websocket` passes socket.io's WebSocket upgrades through (Apache 2.4.47+).
- `LimitRequestBody 0` removes the body limit (1 GB by default since Apache 2.4.54), so ShareX and scripts can upload large files in one request.
- `RequestHeader set X-Forwarded-Proto` tells Fileship the protocol. Apache adds `X-Forwarded-For` (with the client appended) and `X-Forwarded-Host` by itself.
- `RequestHeader unset X-Fileship-Client-IP` removes a header Fileship trusts from proxies, so a visitor cannot send their own and pretend to have another IP address. Fileship then uses the last `X-Forwarded-For` entry, which Apache added.

## HTTPS with Certbot

Start with the HTTP configuration above, point your domain's DNS at the server and open ports 80 and 443. Then:

```sh
sudo apt install certbot python3-certbot-apache
sudo certbot --apache -d files.example.com
```

Certbot creates a copy of the virtual host for port 443 with the certificate, adds a redirect from HTTP and renews the certificate automatically. Change `X-Forwarded-Proto` to `"https"` in the new `fileship-le-ssl.conf`. Written by hand, the HTTPS side looks like this (`a2enmod ssl` first):

```apache
<VirtualHost *:80>
    ServerName files.example.com
    Redirect permanent / https://files.example.com/
</VirtualHost>

<VirtualHost *:443>
    ServerName files.example.com

    SSLEngine on
    SSLCertificateFile /etc/letsencrypt/live/files.example.com/fullchain.pem
    SSLCertificateKeyFile /etc/letsencrypt/live/files.example.com/privkey.pem

    ProxyPreserveHost On
    ProxyRequests Off
    ProxyTimeout 300
    LimitRequestBody 0

    RequestHeader set X-Forwarded-Proto "https"
    RequestHeader unset X-Fileship-Client-IP

    ProxyPass / http://127.0.0.1:3000/ upgrade=websocket
    ProxyPassReverse / http://127.0.0.1:3000/
</VirtualHost>
```

## Apache older than 2.4.47

Without `upgrade=websocket`, send `/socket.io/` WebSocket requests through `mod_rewrite` instead (`a2enmod rewrite`) and drop the option from `ProxyPass`:

```apache
RewriteEngine On
RewriteCond %{HTTP:Upgrade} =websocket [NC]
RewriteRule ^/socket\.io/(.*) ws://127.0.0.1:3000/socket.io/$1 [P,L]

ProxyPass / http://127.0.0.1:3000/
ProxyPassReverse / http://127.0.0.1:3000/
```

This variant was not tested.

## Behind another TLS terminator

When a load balancer or CDN in front of Apache terminates HTTPS, set `X-Forwarded-Proto` to `"https"` in the port 80 virtual host (Apache cannot see the original protocol), or set **Link protocol** to **Always HTTPS** (see [Link protocol](overview.md#link-protocol)).

## Apache in Docker

The official `httpd` image ships with the modules but leaves them off. Uncomment the `LoadModule` lines of `mod_proxy`, `mod_proxy_http`, `mod_proxy_wstunnel` and `mod_headers` in its `httpd.conf`, include the virtual host, and reach the host's port 3000 through `host.docker.internal` (`extra_hosts: ['host.docker.internal:host-gateway']`), like the [Nginx example](nginx.md#nginx-in-docker).

## Check it

Follow [After the proxy works](overview.md#after-the-proxy-works).

> [!NOTE]
> Not tested yet: the configurations on this page follow Apache's documentation, and runs in Docker against a Fileship instance (login, an upload larger than the chunk size, a single-request upload, live updates and the client IP) are still to be done.
