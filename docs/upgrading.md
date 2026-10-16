# Upgrading to 6.0

Fileship 6.0 introduces a redesigned dashboard and renames its environment variables so they can be applied at runtime. Before updating:

**1. Pull the latest project files**, which include the updated `docker-compose.yml`:

```sh
cd Fileship
git pull
```

**2. Rename any of these variables you set yourself:**

| 5.x                    | 6.0                         |
| ---------------------- | --------------------------- |
| `DATA_DIRECTORY`       | `NUXT_DATA_DIRECTORY`       |
| `REMOVE_EXIF_DATA`     | `NUXT_REMOVE_EXIF_DATA`     |
| `AVATAR_MAX_SIZE`      | `NUXT_AVATAR_MAX_SIZE`      |
| `LOG_LEVEL`            | `NUXT_LOG_LEVEL`            |
| `TELEMETRY_ENABLED`    | `NUXT_TELEMETRY_ENABLED`    |
| `UPDATER_URL`          | `NUXT_UPDATER_URL`          |
| `UPDATER_SECRET`       | `NUXT_UPDATER_SECRET`       |
| `AI_QUEUE_CONCURRENCY` | `NUXT_AI_QUEUE_CONCURRENCY` |
| `AI_INFERENCE_THREADS` | `NUXT_AI_INFERENCE_THREADS` |
| `AI_JOB_LEASE_MS`      | `NUXT_AI_JOB_LEASE_MS`      |

`AI_VIDEO_MAX_FRAMES`, `AI_VIDEO_SAMPLE_INTERVAL_SECONDS` and `AI_JOB_DEQUEUE_RETRIES` were removed: the AI preset (`NUXT_AI_PRESET`, `low`, `medium` or `high`) now sets the video frames, and the queue no longer needs retries.

Most of these can now be managed in **Administration → Instance** instead, so you may not need to set them at all. The upload chunk size must now be at least 5 MB; a smaller `NUXT_PUBLIC_FILE_CHUNK_SIZE` falls back to the default of 25 MB.

**3. Recreate the stack:**

```sh
docker compose pull
docker compose up -d --remove-orphans
```

<details>
<summary><strong>Moving from the 5.x compose file</strong></summary>

Every 5.x installation runs a single `fileship` service. The new `docker-compose.yml` you pulled in step 1 adds the updater agent, the proxy and the two app containers. If you don't have a `.env` file with a `SECRET_TOKEN` yet, create one first:

```sh
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
```

This migration restarts Fileship one last time. Later updates do not.

</details>

## AI data

v6 replaces the AI models, the OCR engine and the sensitive content rules. On the first start the old embeddings, OCR text and the sensitive flags that no longer apply are removed, and every file is analyzed again in the background while AI is enabled. Full-text search by file name and caption keeps working in the meantime.
