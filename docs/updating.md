# Updating Fileship

Fileship updates itself without downtime. A small built-in proxy ([Caddy](https://caddyserver.com/)) listens on port `3000` and always routes requests to a healthy container. During an update, the updater agent starts the new version in a second container (`fileship-blue` / `fileship-green`), waits until it is healthy and then stops the old one. If the new version fails to start, it is stopped and the current version keeps running. Updates also wait for any running backup restore to finish.

## One-click update

When a new release is available, admins see an update card in the sidebar. Super admins can click **Update** to start it and follow its progress live.

## Scheduled updates

Automatic updates are enabled by default and run every day at 3:00 AM in the updater agent's time zone (UTC unless you set `TZ` on it). You can change or disable them in `docker-compose.yml`:

```yaml
updater-agent:
    environment:
        AUTOUPDATER_ENABLED: true # set to false to disable scheduled updates
        AUTOUPDATER_CRON: '0 3 * * *' # cron expression (default: every day at 3:00 AM)
```

Coming from 5.x? See [Upgrading to 6.0](upgrading.md).
