---
title: Registration and invites
description: Decide who can get an account on Fileship - admins, invites, self-registration or single sign-on - and protect the forms with Cloudflare Turnstile.
---

# Registration and invites

There are four ways to get an account on Fileship:

| Way                                                  | Who starts it | Needs                                 |
| ---------------------------------------------------- | ------------- | ------------------------------------- |
| [An admin creates it](#accounts-created-by-an-admin) | an admin      | nothing                               |
| [An invite](#invites)                                | an admin      | [email](email.md)                     |
| [Self-registration](#self-registration)              | the person    | **Allow registration** on             |
| [Single sign-on](single-sign-on.md)                  | the person    | a provider and **Create accounts** on |

## Accounts created by an admin

On **Administration → Users**, **New user** creates an account with a username (3 to 24 characters), a password (8 to 48 characters), **Permissions** and **Limits** (**Backups** and **Storage (MB)**, -1 for unlimited). Only super admins can create other super admins. See [Users and permissions](../features/users.md).

## Invites

**Invite** on the **Users** page sends an email with a link to create an account. You choose the permissions, the storage limit and the **Email language**.

- The link works once and expires after 7 days.
- It works even while self-registration is off.
- The invited address becomes the account's confirmed email, unless another account already uses it.
- Inviting the same address again replaces the old invite.
- **Pending invites** are listed above the users and can be withdrawn with **Withdraw**.

Inviting someone with admin rights asks you to confirm it's you first. Invites need [email](email.md) to be set up.

## Self-registration

Self-registration is off by default. Turn on **Allow registration** in **Administration → Instance → Registration** (or set `NUXT_PUBLIC_REGISTRATION_ENABLED=true`) to show a sign up link on the login page.

| Field                  | Default                                 | What it does                                              |
| ---------------------- | --------------------------------------- | --------------------------------------------------------- |
| **Permissions**        | Upload files, Take notes, Shorten links | What new accounts can do. Admin rights are never granted. |
| **Storage limit (MB)** | -1                                      | -1 means unlimited.                                       |

Sign ups are rate limited to 5 per IP address per hour. On a public instance, set a storage limit and turn on [Turnstile](#turnstile) as well.

## Turnstile

[Cloudflare Turnstile](https://www.cloudflare.com/products/turnstile/) puts a bot check on the login and sign up forms.

1. In the Cloudflare dashboard, open **Turnstile → Add widget** and add your Fileship domain as a hostname.
2. Copy the **Site key** and **Secret key** into **Administration → Instance → Turnstile** (or `NUXT_PUBLIC_TURNSTILE_SITE_KEY` and `NUXT_TURNSTILE_SECRET_KEY`).
3. Save, then log out and back in to check it.

Turnstile only turns on when both keys are set. A wrong secret, or a domain that is not listed in the widget, blocks every login with **The security check failed, try again**. If that locks you out, see [Locked out by Turnstile](../troubleshooting/login-and-security.md#locked-out-by-turnstile).

## Login protection

Without Turnstile, Fileship still limits login attempts: 10 per IP address and username every 15 minutes (**Too many attempts, try again later**). This relies on the [client IP](../proxy/overview.md#client-ip); behind a misconfigured proxy every visitor shares one address and one limit.
