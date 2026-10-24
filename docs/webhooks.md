# Webhooks

Webhooks post your Fileship events to Discord, Slack or a service of your own. Add them in **Settings → Integrations → Webhooks**: give the webhook a name, paste its URL, pick a format and the events you want. Every user can add up to 10 webhooks.

## Formats

| Format      | Use it for                                                                                                                                        |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Discord** | A Discord channel webhook (_Channel settings → Integrations → Webhooks_). Fileship sends an embed in your accent color.                           |
| **Slack**   | A Slack [incoming webhook](https://api.slack.com/messaging/webhooks). Fileship sends a block message.                                             |
| **JSON**    | Your own service. Fileship sends a versioned JSON body and signs it, see [JSON payload](#json-payload) and [verifying](#verifying-the-signature). |

Discord and Slack messages are written in your Fileship language.

## Events

| Event                | Sent when                                                                                                         |
| -------------------- | ----------------------------------------------------------------------------------------------------------------- |
| `file.uploaded`      | A file is uploaded from the dashboard, the Windows app, ShareX, the scripts or a visitor through your inbox.      |
| `file.deleted`       | Files are moved to the trash, deleted for good, expire or reach their view limit. One event covers a bulk delete. |
| `storage.almostFull` | An upload takes your storage past 90% of your usable space. Accounts without a storage limit never send it.       |
| `link.created`       | A short link is created.                                                                                          |
| `note.created`       | A note is created.                                                                                                |

**Send test** in a webhook's menu sends a `webhook.test` event right away and shows the result.

## What is sent

Webhook messages often end up in shared channels, so they only contain what a visitor could see:

- Files that are sensitive, in a private folder, expired or not analyzed yet (while AI is on) are sent with their link name (`fileName`) instead of their real name and without a preview. Their `visibility` is `hidden`.
- Password protected files are sent with their link name and without a preview. Their `visibility` is `locked`.
- Password protected short links are sent without their destination, and private notes without their title.

While AI is on, an uploaded image or text file waits up to 30 minutes for its analysis, so a file that turns out to be sensitive is never announced with its name or preview.

## Delivery

Events are queued in the database and sent in the background, so they never slow down an upload. Each webhook gets one request at a time, oldest first.

- A request times out after 10 seconds.
- Network errors, timeouts, `408`, `429` and `5xx` responses are retried after 10 seconds, 1 minute, 5 minutes and 30 minutes (5 attempts). Other responses fail at once.
- On `429`, Fileship waits for `Retry-After` (or Discord's `retry_after`) before sending anything else to that webhook.
- Any `2xx` response counts as delivered.
- **Deliveries** in a webhook's menu shows its last 20 deliveries with the status code, the error and the start of the response.
- After 5 failed deliveries in a row the webhook is turned off and its waiting deliveries are dropped. Fix the URL, send a test and turn it back on.

## Security

- Webhook URLs and signing secrets are stored encrypted with `data/secret.key`. After saving, the dashboard only shows the host. Leave the URL or secret empty when editing to keep the current one.
- Webhooks can't reach loopback, private, link-local and other reserved addresses. Fileship checks the address after resolving the host name, for every request and every redirect (up to 3 redirects, the request is sent again with the same method and body). A super admin can allow private addresses in **Administration → Instance → Webhooks**, for example to reach a service on the same network. Only do this if you trust every user.
- **Send test** is limited to 5 requests per minute.

## JSON payload

Every request is a `POST` with `Content-Type: application/json` and these headers:

| Header                 | Value                                                                      |
| ---------------------- | -------------------------------------------------------------------------- |
| `User-Agent`           | `Fileship-Webhooks/<version>`                                              |
| `X-Fileship-Event`     | The event, for example `file.uploaded`                                     |
| `X-Fileship-Delivery`  | The delivery id. It stays the same when a delivery is retried.             |
| `X-Fileship-Timestamp` | Unix time in seconds when the request was signed                           |
| `X-Fileship-Signature` | `sha256=` and the hex HMAC-SHA256 of `<timestamp>.<body>` with your secret |

The body always has the same envelope. `version` is `1`; fields may be added to version 1, but none are removed or renamed without a new version.

```json
{
    "version": 1,
    "id": "3f95be39-00b1-44a8-bdeb-a93d474bca62",
    "event": "file.uploaded",
    "createdAt": "2026-10-24T13:06:53.678Z",
    "webhook": { "id": "f09d4627-ba06-4d91-967a-bbb5eb65eea6", "name": "My service" },
    "user": { "id": "0f421eb9-f99d-4239-9575-9832fe749fe5", "username": "alice" },
    "data": {}
}
```

`id` identifies the event and is the same for every webhook it is sent to. `data` depends on the event:

### `file.uploaded`

```json
{
    "source": "upload",
    "file": {
        "id": "b3b2f7c3-ff1f-4dff-8f28-143bc3dbb79b",
        "fileName": "usTZVqtR.png",
        "name": "holiday.png",
        "mimeType": "image/png",
        "size": 1410,
        "visibility": "public",
        "url": "https://files.example.com/u/usTZVqtR.png",
        "viewUrl": "https://files.example.com/view/usTZVqtR.png",
        "thumbnailUrl": "https://files.example.com/u/usTZVqtR.png/thumbnail",
        "createdAt": "2026-10-24T13:06:53.648Z"
    }
}
```

`source` is `upload` or `inbox`. `visibility` is `public`, `locked` or `hidden` (see [What is sent](#what-is-sent)); `thumbnailUrl` is `null` unless the file is public and has a preview. `size` is in bytes.

### `file.deleted`

```json
{
    "reason": "trashed",
    "count": 1,
    "files": [
        {
            "id": "b3b2f7c3-ff1f-4dff-8f28-143bc3dbb79b",
            "fileName": "usTZVqtR.png",
            "name": "holiday.png",
            "mimeType": "image/png",
            "size": 1410,
            "visibility": "public"
        }
    ]
}
```

`reason` is `trashed`, `deleted`, `expired` or `maxViews`. `files` lists up to 100 files; `count` is the total.

### `storage.almostFull`

```json
{
    "storage": {
        "used": 954206,
        "limit": 1048576,
        "percent": 91,
        "url": "https://files.example.com/dashboard/settings/storage"
    }
}
```

### `link.created`

```json
{
    "link": {
        "id": "b5948d7f-d2f1-444a-ad61-fb5f896a4081",
        "code": "4qvqwK",
        "url": "https://files.example.com/s/4qvqwK",
        "target": "https://example.com/landing",
        "visibility": "public",
        "maxClicks": 5,
        "expiresAt": "2026-10-25T13:06:53.953Z",
        "createdAt": "2026-10-24T13:06:53.953Z"
    }
}
```

`target` is `null` and `visibility` is `locked` for password protected links. `maxClicks` is `0` without a limit.

### `note.created`

```json
{
    "note": {
        "id": "f27dbc2a-563c-4b06-81b2-f77c4b67002a",
        "title": "Meeting notes",
        "url": "https://files.example.com/note/f27dbc2a-563c-4b06-81b2-f77c4b67002a",
        "visibility": "public",
        "createdAt": "2026-10-24T13:06:53.979Z"
    }
}
```

Private notes have `visibility` `hidden` and `title` and `url` set to `null`.

### `webhook.test`

```json
{ "webhook": { "id": "f09d4627-ba06-4d91-967a-bbb5eb65eea6", "name": "My service" } }
```

## Verifying the signature

Compute the HMAC-SHA256 of the timestamp, a dot and the raw request body with your secret, and compare it with `X-Fileship-Signature` in constant time. Reject requests whose timestamp is more than 5 minutes off, and use `X-Fileship-Delivery` to ignore a delivery you have already handled. Use the raw body: parsing and serializing the JSON again changes it.

Node.js:

```js
import { createHmac, timingSafeEqual } from 'node:crypto';

export const verifyFileship = (secret, headers, rawBody) => {
    const timestamp = headers['x-fileship-timestamp'];
    const signature = headers['x-fileship-signature'] ?? '';
    const expected = `sha256=${createHmac('sha256', secret).update(`${timestamp}.${rawBody}`).digest('hex')}`;

    return (
        Math.abs(Date.now() / 1000 - Number(timestamp)) <= 300 &&
        signature.length === expected.length &&
        timingSafeEqual(Buffer.from(signature), Buffer.from(expected))
    );
};
```

Python:

```python
import hashlib, hmac, time

def verify_fileship(secret: str, headers, raw_body: bytes) -> bool:
    timestamp = headers["X-Fileship-Timestamp"]
    expected = "sha256=" + hmac.new(
        secret.encode(), timestamp.encode() + b"." + raw_body, hashlib.sha256
    ).hexdigest()

    return abs(time.time() - int(timestamp)) <= 300 and hmac.compare_digest(
        expected, headers.get("X-Fileship-Signature", "")
    )
```

Answer with any `2xx` status within 10 seconds. If your work takes longer, queue it and answer first.
