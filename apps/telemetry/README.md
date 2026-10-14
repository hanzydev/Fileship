# Fileship Telemetry

The panel that receives the anonymous reports sent by Fileship instances and turns them into statistics. Only the person running it can open it; there are no public pages.

## 📊 How the numbers are calculated

- Each instance keeps **one report per UTC day**. A newer report on the same day replaces the older one, so restarts and blue/green updates never count twice.
- An instance is **active** on a day when it reported in the 7 days up to and including it. Totals add up the latest report of every active instance.
- The statistics of each day are computed once and kept, so pages read ready rows instead of every report. The current and the previous day are refreshed every hour.
- Growth compares today with 30 days earlier.
- Storage, links, views and feature usage come only from Fileship 6 and later; older versions only send counts and system information.

## 🗑️ Retention

- An instance that sends no report for `NUXT_INSTANCE_RETENTION_DAYS` days (90 by default) is deleted together with its reports. If it reports again later, it starts as a new instance.
- Reports of active instances are kept for 13 months.
- Daily statistics hold only totals and are kept, so the charts keep the history of deleted instances.
- IP addresses are never stored.

## ⚙️ Environment

| Variable                       | Description                                                          |
| ------------------------------ | -------------------------------------------------------------------- |
| `DATABASE_URL`                 | PostgreSQL connection string (required)                              |
| `NUXT_ADMIN_PASSWORD`          | Password for the panel (required, the panel stays locked without it) |
| `NUXT_INSTANCE_RETENTION_DAYS` | Days without a report before an instance is deleted (default: 90)    |

## 🐳 Running

```yaml
services:
    telemetry:
        image: ghcr.io/hanzydev/fileship-telemetry:latest
        restart: unless-stopped
        environment:
            DATABASE_URL: postgresql://telemetry:password@postgres:5432/telemetry
            NUXT_ADMIN_PASSWORD: change-me
        ports:
            - 3000:3000
        depends_on:
            - postgres

    postgres:
        image: postgres:17-alpine
        restart: unless-stopped
        environment:
            POSTGRES_USER: telemetry
            POSTGRES_PASSWORD: password
            POSTGRES_DB: telemetry
        volumes:
            - ./postgres:/var/lib/postgresql/data
```

Migrations run on start. Fileship instances send their reports to `POST /api/v2/report`; instances older than 6.0 use `POST /api/event`, which keeps working.

## 🧑‍💻 Development

```sh
cp apps/telemetry/.env.example apps/telemetry/.env
pnpm --filter telemetry dev
```

The panel runs on port 3002. Point a local dashboard at it with `NUXT_TELEMETRY_URL=http://localhost:3002` to send reports from development.
