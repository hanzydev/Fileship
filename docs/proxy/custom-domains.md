---
title: Custom domains
description: Give Fileship links other domains (for example i.example.com), with the DNS records and reverse proxy settings they need.
---

# Custom domains

Each user can add domains that Fileship uses when it builds links to their files and short links, for example a short `i.example.com` next to the dashboard at `files.example.com`.

## Add a domain in Fileship

Open **Settings → Sharing → Domains**, type a domain (such as `i.example.com`, without `https://`) and choose **Add**. Up to 50 domains per user. With several domains, one is picked at random for each link. With none, links use the address you uploaded from.

Fileship does not check the domain. You make it reach Fileship with the steps below; until then, links on it do not open.

## Point the domain at Fileship

1. **DNS:** add an `A` (and `AAAA`) record for the domain with your server's address, or a `CNAME` to your dashboard's domain.
2. **Reverse proxy:** answer the new domain with the same configuration as the dashboard, so it reaches Fileship on port 3000 and gets its own certificate.

Caddy:

```caddy
files.example.com, i.example.com {
    reverse_proxy 127.0.0.1:3000 {
        header_up X-Fileship-Client-IP {client_ip}
    }
}
```

Nginx: add the domain to `server_name` and to Certbot:

```nginx
server_name files.example.com i.example.com;
```

```sh
sudo certbot --nginx -d files.example.com -d i.example.com
```

Traefik: ``rule: Host(`files.example.com`) || Host(`i.example.com`)``. Apache: `ServerAlias i.example.com`.

The proxy must keep the `Host` header (every example in these pages does), so Fileship knows which domain a request came to.

## Things to know

- Every Fileship address serves the whole app; a custom domain is not limited to file links. Any page on it works, including the login page.
- The browser keeps the login separately per domain. Open the dashboard at its own address; on a custom domain you would have to log in again, and while logged out you count as a visitor (your views are counted).
- Single sign-on and emails always use the **Dashboard address**, not custom domains.
- Link protocol follows the request or the **Link protocol** setting, so give custom domains HTTPS too.
