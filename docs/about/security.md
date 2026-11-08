---
title: Security
description: How to report a security problem in Fileship, and an overview of how Fileship protects accounts, files and links.
---

# Security

## Reporting a vulnerability

Please do not open a public issue for security problems. The [security policy](../../SECURITY.md) explains how to report one privately and which versions receive fixes.

## How Fileship protects you

- **Accounts:** passwords hashed with Argon2, optional two-factor authentication and passkeys, [single sign-on](../configuration/single-sign-on.md), rate limited logins and confirmation with your password, code or passkey before sensitive actions. Sessions are random tokens stored only as hashes. See [Users and permissions](../features/users.md).
- **Files:** uploads are served with headers that stop them from running scripts on your domain; HTML, SVG, XML and JavaScript are downloaded instead of shown, and PDFs are drawn without scripts. Visitors never see files in the trash, expired files, sensitive files or files in private folders. See [Sharing options](../features/sharing.md).
- **Secrets:** storage credentials, upload tokens, webhook URLs and the email and single sign-on secrets are encrypted with `data/secret.key` and never sent back to the browser.
- **Webhooks** only reach public addresses unless a super admin allows private ones.
- **Requests:** logged-in requests that a browser marks as cross-site are refused, and request bodies are size limited.

For setting up a server safely, also read [Reverse proxy overview](../proxy/overview.md) (client IPs and headers) and [Backing up the database](../backups/database.md).
