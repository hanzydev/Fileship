---
title: AI problems
description: Fix Fileship's AI analysis - model downloads, out of memory and killed processes, a stuck queue, failed files and "AI features are turned off".
---

# AI problems

The **AI** section of **Administration → Instance** shows the **Queue**: how many files are waiting, running and failed, and how much space the models use. The server output shows errors from AI jobs; the models run in the AI service, whose output is `docker compose logs fileship-ml` with Docker and part of the Fileship log without Docker.

## The AI service is not reachable

Super admins see **AI service unreachable** in the sidebar. Files wait for their analysis and nothing is lost; once the service answers again, the queue continues by itself.

- **Docker, updated from the dashboard or by scheduled updates:** the updater agent adds the `fileship-ml` container after it has updated itself. Wait a few minutes, or restart it with `docker compose restart updater-agent`.
- **Docker, updated by hand:** the Compose file needs the `fileship-ml` service. Run `git pull`, `docker compose pull` and `docker compose up -d` in the Fileship folder.
- **Without Docker:** Fileship starts the service from `apps/ml/.venv`. Install [uv](../install/without-docker.md#1-install-nodejs-pnpm-and-the-build-tools) if it is missing, run `pnpm --filter ml build` as the user that runs Fileship and restart Fileship.
- **Still unreachable:** read the service's output (`docker compose logs fileship-ml`, or the Fileship log without Docker). On a machine that is out of memory, see [Out of memory](#out-of-memory).

## Models don't download

The AI service downloads the models from Hugging Face the first time they are needed and are kept in `data/models` (up to 3 GB). Without internet access, or with `huggingface.co` blocked, jobs fail and retry.

- Allow outgoing HTTPS to `huggingface.co` and its download hosts.
- Or copy `data/models` from another installation with the same preset.
- Check free disk space.

## Out of memory

The AI service is killed (exit code 137, or `Killed` in its log) while files are analyzed. It restarts by itself and Fileship keeps running; a file that keeps crashing the service is marked failed after three tries.

- Choose a smaller **Preset**: **Low (4 GB RAM)**, **Medium (6 GB RAM)** or **High (8 GB RAM)** are the memory each needs.
- Keep **Parallel jobs** at 1 on small servers. Each parallel job needs its own memory.
- Turn **AI features** off if the server has 2 GB or less.

After a crash, the running jobs are tried again; a file that fails three times is marked failed.

## The server is slow while files are analyzed

Analysis runs on the CPU. Lower **Threads per job** and **Parallel jobs** so the dashboard keeps some cores.

## The queue is stuck

- **Waiting** files but nothing **running**: AI is off for the instance, or the files' owners turned it off in **Settings → AI**.
- **Running** for a long time: a large video or PDF takes a while. A job whose process died is picked up again after **Job heartbeat timeout** (2 minutes by default).
- **Failed** files wait for **Retry failed**. Failed jobs are cleared after 30 days, and those files are then tried again.

## Search doesn't find a file

- While a file is still being analyzed, only full-text search by name finds it.
- Smart search only shows results that clearly match, so vague queries can return few or none. Try a more specific description, or turn off **Smart search** to search names.
- After changing the preset, files are analyzed again in the background; results improve as it finishes.

## AI features are turned off

You used an AI feature (such as text capture in the Windows app, which uses OCR) while AI is off for the instance or for your account. A super admin turns it on in **Administration → Instance → AI**, and you in **Settings → AI**.

## A file was wrongly marked as sensitive

Choose **Mark as not sensitive** from the file's menu. Your choice is kept when the file is analyzed again. See [Sensitive content](../ai.md#sensitive-content).
