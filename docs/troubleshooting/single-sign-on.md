---
title: Single sign-on problems
description: Fix OpenID Connect logins in Fileship - wrong address, discovery failures, Google's invalid_client, no linked account and the verification pop-up.
---

# Single sign-on problems

When a provider login fails, the server output has the reason (`Single sign-on failed: …`). Read it first:

```sh
docker compose logs --tail=100 fileship-blue fileship-green | grep -i "single sign-on"
```

## Open Fileship at its dashboard address to use single sign-on

The redirect URI is built from the **Dashboard address**, so single sign-on only works there. You opened Fileship at another address (an IP, `localhost`, a custom domain). Open it at the dashboard address. A login started elsewhere is sent there once; linking and confirming refuse other addresses (**Open Fileship at … to link single sign-on**).

## The login button doesn't appear

The button appears once **Enable single sign-on** is on and the **Dashboard address**, the **Issuer URL** and the **Client ID** are set (**Set the dashboard address in General first.**).

## Couldn't read the discovery document

**Test** (or the login) could not load `<issuer>/.well-known/openid-configuration`. The message after the colon says why:

- **A 404:** the issuer URL is wrong. Copy it exactly from the provider, with or without the trailing slash as the provider publishes it. Open the discovery URL in a browser to check.
- **A timeout or connection error:** Fileship's server cannot reach the provider. With Docker, `localhost` inside the container is the container itself; use the provider's real address or a name on the same Docker network. The issuer must be the same address the browser uses.
- **A certificate error:** the provider uses a certificate Fileship's server does not trust (self-signed). Use a trusted certificate, or an `http://` issuer on a private network.
- **An issuer mismatch:** the `issuer` inside the document differs from what you entered. Use the value from the document.

## Single sign-on isn't available right now

The provider could not be reached during a login. Check the server output, then use **Test** in **Administration → Instance → Single sign-on**.

## The provider says the redirect URI is invalid

The redirect URI at the provider must be exactly the one under **Redirect URI**, such as `https://files.example.com/api/auth/oidc/callback`. A different scheme (`http` instead of `https`), host or a trailing slash makes it fail. If it shows `http://` although you use HTTPS, fix the **Dashboard address**.

## Google says invalid_client

"The OAuth client was not found" from Google after you consented. Fileship 6.3.0 encoded the client ID in a way Google does not accept; update to 6.3.1 or newer. On newer versions, check that the client ID and secret were copied completely.

## No account is linked to this account

The login worked at the provider, but Fileship found no account for it:

- Turn on **Create accounts** to create accounts for new people, or
- log in another way and link the provider in **Settings → Security → Single sign-on**, or
- give the Fileship account the same **confirmed** email address and make sure the provider marks it as verified (`email_verified`). Microsoft Entra ID never sends it.

**The account with this email address is already linked to a different … account** means the matching Fileship account is linked to another account at the same provider. Unlink that one first.

## The login took too long or was started in another window

The login has to finish within 10 minutes, in the same browser it started in. Start again from the login page. Browsers that block third-party cookies are fine, but a login started in one browser and finished in another (for example a link opened in another app) fails.

## Confirming an action opens nothing

Accounts that only log in through the provider confirm sensitive actions in a small pop-up window. Allow pop-ups for the dashboard address. With a provider that keeps its own session, the pop-up may close at once and confirm (some providers ignore `max_age=0`).

## Set a password or add a passkey first

Fileship refuses to unlink single sign-on while the account has no other way to log in. Set a password in **Settings → Security** or add a passkey, then unlink. While **Password and passkey login** is off, unlinking is refused for everyone except super admins.

## Too many attempts

Logins and callbacks are limited to 30 per IP address every 15 minutes. Wait, or restart Fileship.
