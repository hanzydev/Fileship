# Installation

## Requirements

|          | AI disabled | AI preset `low` | AI preset `medium` | AI preset `high` |
| -------- | ----------- | --------------- | ------------------ | ---------------- |
| **CPU**  | 2 cores     | 2 cores         | 4 cores            | 4 cores or more  |
| **RAM**  | 2 GB        | 4 GB            | 6 GB               | 8 GB             |
| **Disk** | 4 GB        | 6 GB            | 7 GB               | 7.5 GB           |

The disk sizes include about 1.8, 2.6 and 3 GB of AI models. Add the space for uploaded files kept on the local disk. AI runs on the CPU, so more cores analyze uploads faster; no GPU is needed. Both x86-64 and ARM64 are supported. See [AI presets](ai.md#ai-presets) for what each preset does.

You also need [Docker](https://www.docker.com/) and [Docker Compose](https://docs.docker.com/compose/).

PostgreSQL 16 is included in the Compose stack.

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

Fileship is now available at `http://localhost:3000`.

## Default credentials

- Username: `admin`
- Password: `password`

> [!IMPORTANT]
> Change the password right after your first login (**Settings → Security**).

## Browser compatibility

Fileship targets current versions of Chrome, Edge, Firefox, Brave and Zen Browser. Safari is supported through compatibility fallbacks, but a Chromium- or Firefox-based browser gives the best experience.

## Web server configuration (optional)

To serve Fileship on your own domain, put a reverse proxy in front of it. An [Nginx](https://nginx.org/) example:

```nginx
map $http_x_forwarded_proto $the_scheme {
    default $scheme;
    ~. $http_x_forwarded_proto;
}

server {
    listen 80;
    server_name <domain>;

    client_max_body_size 100M;

    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;

    proxy_set_header X-Forwarded-Proto $the_scheme;

    location / {
        proxy_pass http://127.0.0.1:3000;
    }

    location /socket.io/ {
        proxy_pass http://127.0.0.1:3000;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

Nginx points to `127.0.0.1:3000`, the built-in proxy. WebSocket upgrades and the `X-Forwarded-*` headers pass through, so Fileship still sees the real client IP. The built-in proxy has no upload size limit of its own, so `client_max_body_size` is the only one that applies. Uploads are sent in chunks (25 MB by default), so the limit only needs to be larger than the chunk size.
