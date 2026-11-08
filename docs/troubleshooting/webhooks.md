---
title: Webhook problems
description: Fix Fileship webhooks - private address errors, timeouts, failed deliveries and webhooks that turned themselves off.
---

# Webhook problems

Each webhook's menu has **Show deliveries**: the recent deliveries with their status and the receiver's answer. **Send test** sends a `webhook.test` event right away.

## This URL points to a private or local address

Webhooks may only reach public addresses, so a user cannot use them to reach services inside your network. A URL whose host resolves to `localhost`, a private network (`10.x`, `172.16–31.x`, `192.168.x`) or a link-local address is refused, when you save it and again for every request and redirect.

For a receiver on your own network, a super admin turns on **Allow private addresses** in **Administration → Instance → Webhooks**. Only do that if you trust every user.

## Deliveries fail with a timeout

The receiver must answer within 10 seconds. Answer quickly and do slow work afterwards. Timeouts, network errors, `408`, `429` and `5xx` answers are retried after 10 seconds, 1 minute, 5 minutes and 30 minutes; a `Retry-After` header on `429` is respected.

## The webhook turned itself off

After several failed deliveries in a row, Fileship turns the webhook off, cancels its pending deliveries and shows why on the Integrations page. Fix the receiver, then turn the webhook back on with its switch.

## Discord or Slack shows nothing

- The URL must be the channel's webhook URL (Discord: **Channel settings → Integrations → Webhooks**; Slack: an incoming webhook).
- Discord rejects deleted webhooks with `404`; create a new one.
- Messages leave out names and previews of locked, sensitive or hidden files on purpose; see [What is sent](../webhooks.md#what-is-sent).

## The signature doesn't match

The `JSON` format signs `<timestamp>.<body>` with your secret. Use the raw body as received, before parsing it. See [Verifying the signature](../webhooks.md#verifying-the-signature).

## Use a secret of at least 16 characters / You can add up to 10 webhooks

Secrets must be 16 to 256 characters (**Generate** makes one). Each user can have up to 10 webhooks.
