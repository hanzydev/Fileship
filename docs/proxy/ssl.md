---
title: SSL certificates
description: Get HTTPS certificates for Fileship with Caddy, Certbot, Traefik, win-acme or Cloudflare, and make sure links use https.
---

# SSL certificates

Fileship itself speaks plain HTTP; HTTPS is handled by your [reverse proxy](overview.md). HTTPS is needed for passkeys, installing the app on a phone, the Android share target and copying links in some browsers.

## Options

| Proxy                                     | How to get a certificate                                                                          |
| ----------------------------------------- | ------------------------------------------------------------------------------------------------- |
| [Caddy](caddy.md)                         | Automatic. Write your domain in the Caddyfile; Caddy gets and renews a Let's Encrypt certificate. |
| [Nginx](nginx.md)                         | [Certbot](https://certbot.eff.org/): `sudo certbot --nginx -d files.example.com`.                 |
| [Apache](apache.md)                       | Certbot: `sudo certbot --apache -d files.example.com`.                                            |
| [Traefik](traefik.md)                     | A certificate resolver with Let's Encrypt (ACME), configured once.                                |
| [IIS](iis.md)                             | [win-acme](https://www.win-acme.com/), or a certificate you import.                               |
| [Cloudflare Tunnel](cloudflare-tunnel.md) | Cloudflare handles it; nothing to do on your server.                                              |

Let's Encrypt needs your domain to point at the server and port 80 (HTTP challenge) or a DNS provider API (DNS challenge) to be reachable. Certificates last 90 days or less and are renewed automatically by all the tools above.

## Certbot

On Debian and Ubuntu:

```sh
sudo apt install certbot python3-certbot-nginx   # or python3-certbot-apache
sudo certbot --nginx -d files.example.com
sudo certbot renew --dry-run
```

Certbot installs a timer that renews certificates before they expire. The last command checks that renewal works.

## IIS

With [win-acme](https://www.win-acme.com/), run `wacs.exe` as administrator, choose **Create certificate (default settings)**, pick your IIS site and binding, and it installs the certificate and a scheduled task that renews it.

## Cloudflare origin certificates

When Cloudflare's proxy sits in front of your server, you can use a free [origin certificate](https://developers.cloudflare.com/ssl/origin-configuration/origin-ca/) between Cloudflare and your server instead of Let's Encrypt: create it in **SSL/TLS → Origin Server**, install the certificate and key in your proxy (`ssl_certificate` and `ssl_certificate_key` in Nginx), and set the SSL mode to **Full (strict)**. Only Cloudflare trusts these certificates, so visitors must always come through Cloudflare.

## Local and private networks

For a server only reachable on your network:

- Caddy's `tls internal` creates certificates from its own local authority; install Caddy's root certificate on your devices.
- [mkcert](https://github.com/FiloSottile/mkcert) creates a locally trusted certificate for Nginx, Apache or Traefik.
- A DNS challenge (Caddy, Traefik or Certbot with a DNS plugin) gets real Let's Encrypt certificates for a domain that points at a private address.

## Make sure links use https

After HTTPS works, open **Administration → Instance → General**:

- Set **Dashboard address** to the `https://` address.
- Leave **Link protocol** on **Same as the request**. If uploads still return `http://` links, your proxy is not sending `X-Forwarded-Proto: https`; fix the proxy, or choose **Always HTTPS**.

The login cookie is only marked `Secure` when Fileship knows the request was HTTPS, which is another reason to send `X-Forwarded-Proto`.
