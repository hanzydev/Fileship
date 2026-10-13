<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    An open-source, self-hosted file and media management platform.
    <br />
    <br />
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Report a bug</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Request a feature</a>
    ·
    <a href="SECURITY.md">Security policy</a>
  </p>
</div>

## 📸 Screenshots

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Fileship dashboard on desktop" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Fileship files on mobile" width="18%">
</p>

## 🧰 Built With

![Nuxt](https://img.shields.io/static/v1?style=for-the-badge&message=Nuxt&color=222222&logo=Nuxt&logoColor=00DC82&label=)
![Vue.js](https://img.shields.io/static/v1?style=for-the-badge&message=Vue.js&color=222222&logo=Vue.js&logoColor=4FC08D&label=)
![Tailwind CSS](https://img.shields.io/static/v1?style=for-the-badge&message=Tailwind+CSS&color=0B1120&logo=TailwindCSS&logoColor=38BDF8&label=)
![shadcn-vue](https://img.shields.io/static/v1?style=for-the-badge&message=shadcn-vue&color=000000&logo=shadcnui&logoColor=FFFFFF&label=)
![TypeScript](https://img.shields.io/static/v1?style=for-the-badge&message=TypeScript&color=3178C6&logo=TypeScript&logoColor=FFFFFF&label=)
![Prisma](https://img.shields.io/static/v1?style=for-the-badge&message=Prisma&color=2D3748&logo=Prisma&logoColor=FFFFFF&label=)
![PostgreSQL](https://img.shields.io/static/v1?style=for-the-badge&message=PostgreSQL&color=4169E1&logo=PostgreSQL&logoColor=FFFFFF&label=)
![Socket.io](https://img.shields.io/static/v1?style=for-the-badge&message=Socket.io&color=010101&logo=Socket.io&logoColor=FFFFFF&label=)
![Transformers.js](https://img.shields.io/static/v1?style=for-the-badge&message=Transformers.js&color=222222&logo=HuggingFace&logoColor=FFD21E&label=)
![Docker](https://img.shields.io/static/v1?style=for-the-badge&message=Docker&color=2496ED&logo=Docker&logoColor=FFFFFF&label=)
![Caddy](https://img.shields.io/static/v1?style=for-the-badge&message=Caddy&color=1F88C0&logo=Caddy&logoColor=FFFFFF&label=)

## 📦 Features

**Files and sharing**

- Chunked uploads with drag and drop, paste-to-upload and an upload queue
- Folders with cover previews, public sharing and archive downloads; password protected files in a public folder show a lock and open with their password
- Markdown notes with code highlighting, math and public links
- Password protection, expiry dates and view limits for files
- Image compression, EXIF removal, image and video thumbnails, and media details (resolution, duration, frame rate, bitrate, codecs)
- Custom domains and OpenGraph embeds for links shared on Discord and other apps
- Inbox links that let anyone send files straight to your account
- A link shortener with custom short links, passwords, click limits and expiry dates
- A trash that keeps deleted files, folders and notes restorable for a set number of days
- ShareX uploader and URL shortener configs and macOS and Linux scripts, all using a revocable upload token

**Search and AI**

- Smart search in over 50 languages that understands what is in your images, videos and documents
- Image captions, OCR and detection of sensitive content such as contact details, addresses, ID and account numbers, payment cards and secret keys, in any language
- Every model runs locally on your server, and all of it can be turned off

**Accounts and security**

- Multiple users with per-user permissions and storage limits
- Optional self-registration
- Two-factor authentication with authenticator apps and passkeys
- Session management and an audit log
- Cloudflare Turnstile protection for the login and sign up forms
- Personal backups that can be downloaded and restored

**Operations**

- Real-time updates across every open tab and device
- Usage statistics
- Instance settings managed from the dashboard
- Uploads and backups on the local disk, any S3 compatible service, SFTP or FTP
- Zero-downtime updates, one click or on a schedule
- Installable as a Progressive Web App

## 🌐 Browser Compatibility

Fileship targets current versions of Chrome, Edge, Firefox, Brave and Zen Browser. Safari is supported through compatibility fallbacks, but a Chromium- or Firefox-based browser gives the best experience.

## 🚀 Getting Started

### Requirements

- **CPU**: 2 cores
- **RAM**: 6 GB with AI features (without them, less is enough)
- **Disk**: 4 GB, plus space for uploaded files kept on the local disk
- [Docker](https://www.docker.com/) and [Docker Compose](https://docs.docker.com/compose/)

PostgreSQL 16 is included in the Compose stack.

### Installation

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

### Default credentials

- Username: `admin`
- Password: `password`

> [!IMPORTANT]
> Change the password right after your first login (**Settings → Security**).

## ⚙️ Configuration

### Instance settings

Super admins can configure Fileship from the dashboard in **Administration → Instance**. The changes apply immediately, without a restart:

- **General:** site name and link protocol
- **Uploads:** chunk size, avatar size limit, trash retention and EXIF removal
- **Registration:** self-registration, default permissions (upload files, take notes, shorten links) and storage limit
- **Turnstile:** site and secret keys
- **AI:** features on or off, concurrency and video processing
- **Logs and telemetry:** log level and anonymous telemetry
- **Storage:** where uploads and backups are kept

Each of these settings can also be given a default through an environment variable (see [`apps/dashboard/.env.example`](apps/dashboard/.env.example)). A value saved in the dashboard always overrides the environment, and every field can be reset to its default.

### Storage

Fileship keeps uploads and backups in its data directory by default. In **Administration → Instance → Storage** you can add S3 compatible storage (AWS S3, Cloudflare R2, MinIO, Backblaze B2 and others), SFTP or FTP, and choose which one new files go to. Each file stays where it was written, so switching storage does not move anything. Use **Move everything here** on the active storage to move existing files and backups to it.

Uploads stream to the storage chunk by chunk. Thumbnails and temporary files always stay in the data directory. Storage credentials are encrypted with `data/secret.key`, so keep that file together with your data.

On every start, Fileship removes files and backups whose data is missing from their storage. A storage that cannot be reached or looks empty is skipped, so a misconfigured storage never deletes anything.

### Trash

Deleted files, folders and notes move to the trash for 30 days by default. Set **Trash retention** in **Administration → Instance** (or `NUXT_TRASH_RETENTION_DAYS`) to change it; `0` deletes everything immediately. Items in the trash still count toward a user's storage limit until they are deleted for good. Expired files and files that reach their view limit are always deleted immediately.

### Environment-only settings

A few settings can only be set through the environment, because they are needed before the dashboard starts or are fixed when the app is built:

| Variable                                      | Description                                                     |
| --------------------------------------------- | --------------------------------------------------------------- |
| `DATABASE_URL`                                | PostgreSQL connection string (required)                         |
| `NUXT_DATA_DIRECTORY`                         | Local storage, thumbnails and temporary files (default: `data`) |
| `NUXT_UPDATER_URL` / `NUXT_UPDATER_SECRET`    | How the dashboard reaches the updater agent                     |
| `DELETE_EXPIRED_CRON` / `GENERATE_STATS_CRON` | Schedules for expired file cleanup and statistics (build time)  |
| `ENABLE_PWA`                                  | Progressive Web App support (build time)                        |

To set an environment variable with Docker Compose, add it to the shared `x-fileship` block in `docker-compose.yml`, so both the `fileship-blue` and `fileship-green` containers receive it:

```yaml
x-fileship: &fileship
    environment:
        NUXT_PUBLIC_REGISTRATION_ENABLED: 'true'
```

Then apply it with `docker compose up -d`.

## 🔄 Updating Fileship

Fileship updates itself without downtime. A small built-in proxy ([Caddy](https://caddyserver.com/)) listens on port `3000` and always routes requests to a healthy container. During an update, the updater agent starts the new version in a second container (`fileship-blue` / `fileship-green`), waits until it is healthy and then stops the old one. If the new version fails to start, it is removed and the current version keeps running. Updates also wait for any running backup restore to finish.

### One-click update

When a new release is available, admins see an update card in the sidebar. Click **Update** to start it and follow its progress live.

### Scheduled updates

Automatic updates are enabled by default and run every day at 3:00 AM. You can change or disable them in `docker-compose.yml`:

```yaml
updater-agent:
    environment:
        AUTOUPDATER_ENABLED: true # set to false to disable scheduled updates
        AUTOUPDATER_CRON: '0 3 * * *' # cron expression (default: every day at 3:00 AM)
```

### ⚠️ Upgrading to 6.0

Fileship 6.0 introduces a redesigned dashboard and renames its environment variables so they can be applied at runtime. Before updating:

**1. Pull the latest project files**, which include the updated `docker-compose.yml`:

```sh
cd Fileship
git pull
```

**2. Rename any of these variables you set yourself:**

| 5.x                                | 6.0                                     |
| ---------------------------------- | --------------------------------------- |
| `DATA_DIRECTORY`                   | `NUXT_DATA_DIRECTORY`                   |
| `REMOVE_EXIF_DATA`                 | `NUXT_REMOVE_EXIF_DATA`                 |
| `AVATAR_MAX_SIZE`                  | `NUXT_AVATAR_MAX_SIZE`                  |
| `LOG_LEVEL`                        | `NUXT_LOG_LEVEL`                        |
| `TELEMETRY_ENABLED`                | `NUXT_TELEMETRY_ENABLED`                |
| `UPDATER_URL`                      | `NUXT_UPDATER_URL`                      |
| `UPDATER_SECRET`                   | `NUXT_UPDATER_SECRET`                   |
| `AI_QUEUE_CONCURRENCY`             | `NUXT_AI_QUEUE_CONCURRENCY`             |
| `AI_INFERENCE_THREADS`             | `NUXT_AI_INFERENCE_THREADS`             |
| `AI_VIDEO_MAX_FRAMES`              | `NUXT_AI_VIDEO_MAX_FRAMES`              |
| `AI_VIDEO_SAMPLE_INTERVAL_SECONDS` | `NUXT_AI_VIDEO_SAMPLE_INTERVAL_SECONDS` |
| `AI_JOB_LEASE_MS`                  | `NUXT_AI_JOB_LEASE_MS`                  |
| `AI_JOB_DEQUEUE_RETRIES`           | `NUXT_AI_JOB_DEQUEUE_RETRIES`           |

Most of these can now be managed in **Administration → Instance** instead, so you may not need to set them at all. The upload chunk size must now be at least 5 MB; a smaller `NUXT_PUBLIC_FILE_CHUNK_SIZE` falls back to the default of 25 MB.

**3. Recreate the stack:**

```sh
docker compose pull
docker compose up -d --remove-orphans
```

<details>
<summary><strong>Upgrading from an older 5.x setup</strong></summary>

If your installation predates the updater agent or zero-downtime updates (your `docker-compose.yml` has a single `fileship` service), run the steps above as well. The new `docker-compose.yml` adds the updater agent, the proxy and the two app containers. If you don't have a `.env` file with a `SECRET_TOKEN` yet, create one first:

```sh
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
```

This migration restarts Fileship one last time. Later updates do not.

</details>

## 🌐 Web server configuration (optional)

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

## 🤖 AI Features

Fileship can analyze uploads to power smart search, image captions, OCR and sensitive content detection. The models run locally with [Transformers.js](https://huggingface.co/docs/transformers.js); no file ever leaves your server.

AI features are **enabled by default**. Super admins can turn them off in **Administration → Instance**, and each user can opt out in their own settings. The `NUXT_PUBLIC_AI_ENABLED` environment variable sets the instance default.

When AI is disabled:

- no new AI jobs are queued or processed
- smart search falls back to full-text search
- AI-related parts of the interface are hidden

AI processing is CPU-intensive. On small servers, keep **Parallel jobs** at 1 and lower **Inference threads** in the instance settings.

### Models

The models are downloaded from Hugging Face the first time they are needed.

| Task                                   | Model                                                                                                                                                                                      |
| -------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Image and video search                 | [CLIP ViT-B/32](https://huggingface.co/Xenova/clip-vit-base-patch32) with the [multilingual CLIP text encoder](https://huggingface.co/sentence-transformers/clip-ViT-B-32-multilingual-v1) |
| Captions, OCR and document detection   | [Florence-2 base](https://huggingface.co/onnx-community/Florence-2-base)                                                                                                                   |
| Search in captions, OCR and text files | [Multilingual MiniLM](https://huggingface.co/Xenova/paraphrase-multilingual-MiniLM-L12-v2)                                                                                                 |
| Sensitive content                      | [OpenAI Privacy Filter](https://huggingface.co/openai/privacy-filter)                                                                                                                      |

### Smart search

Smart search combines what images and videos show, what their captions and OCR text say, the content of text files, and file names. Queries work in over 50 languages, so `köpek` finds the same photos as `dog`. Only results that clearly match are shown; a query that matches nothing returns nothing.

### Sensitive content

Fileship reads the text of documents and the OCR text of images with the [OpenAI Privacy Filter](https://huggingface.co/openai/privacy-filter) model, which looks at the context instead of fixed patterns, so it works with any country's formats. A file is marked as sensitive when it contains:

- private email addresses, phone numbers or home addresses
- account, ID, tax or social security numbers
- passwords and secret keys
- payment card numbers (checked with the Luhn algorithm), IBANs (checked with their checksum) and the machine-readable lines of passports and ID cards

Public information such as company addresses, support emails, brands and public figures is not sensitive. Faces are not sensitive, and a name or a personal date only counts next to something else, or when they appear together.

The model is loaded when a file is checked and unloaded after five idle minutes. It needs about 2 GB of RAM while loaded. Sensitive files are hidden from public folders and their archives, and can be blurred for you with **Hide sensitive previews** in your AI settings.

If Fileship gets it wrong, choose **Not sensitive** or **Mark as sensitive** from the file's menu. Your choice is kept when the file is checked again.

### Upgrading to v6

v6 replaces the search models and the sensitive content rules. On the first start the old embeddings and the sensitive flags that no longer apply are removed, and every file is processed again in the background while AI is enabled. Full-text search keeps working in the meantime.

## 📊 Telemetry

Fileship collects anonymous usage data to understand how it is used and to guide development. Collection is **optional** and can be turned off at any time.

### What is collected

- **Instance information:** app version, counts of files, folders, notes and users, and the Node.js version
- **System information:** OS platform, release and architecture, CPU count and model, memory and disk usage, and whether Fileship runs in Docker

No personal information, file contents or file names are collected. Each instance is identified only by a random ID. See the [telemetry package](packages/telemetry/README.md) for the full list of fields.

### Opting out

Turn off **Anonymous telemetry** in **Administration → Instance**, or set `NUXT_TELEMETRY_ENABLED` to `false` in the `x-fileship` block of your `docker-compose.yml`.

## ❤️ Contributing

Contributions are welcome. Please read the [contribution guidelines](CONTRIBUTING.md) first. [AGENTS.md](AGENTS.md) describes the architecture and conventions of the codebase.

## ⚖️ License

[MIT](LICENSE)
