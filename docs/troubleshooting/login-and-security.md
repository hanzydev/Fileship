---
title: Login and security problems
description: Fix "Too many attempts", wrong client IPs behind a proxy, "Invalid origin", random logouts, passkey and two-factor problems and a Turnstile lockout.
---

# Login and security problems

## Too many attempts, try again later

Fileship limits login attempts to 10 per IP address and username every 15 minutes, and other sensitive actions similarly. Wait 15 minutes, or restart Fileship to clear the limits (they are kept in memory).

If it happens to everyone at once, Fileship probably sees the same IP address for every visitor; see the next section.

## Everyone has the same IP

The **Logs** page, **Settings → Sessions** and the server output show the same address for everyone, such as `172.18.0.1` or `127.0.0.1`. Then your reverse proxy does not pass the client address, and every visitor shares one rate limit.

- Make the proxy **append** the client to `X-Forwarded-For` (Nginx `$proxy_add_x_forwarded_for`; Apache, Caddy and Traefik do it by default).
- Run the proxy on the same machine or a private network. Fileship only trusts these headers from private and loopback addresses; a proxy on another public server is not trusted.
- With Docker Desktop on Windows or macOS, connections from other devices may all arrive from Docker's own gateway address. Running the proxy on the same machine and sending its header fixes that.

See [Client IP](../proxy/overview.md#client-ip).

## Invalid origin

A logged-in request was refused because the browser marked it as coming from another site (`Sec-Fetch-Site: cross-site`). Fileship refuses such requests to protect against cross-site attacks. You see this when another website, or a page on another domain, tries to change things in Fileship with your login. Use the dashboard at its own address.

Apps and scripts that send the session or upload token in the `Authorization` header are never affected.

## Logged out at random

- **Several Fileship instances (or other apps) on `localhost` with different ports** share cookies, because cookies ignore the port. Logging in to one logs you out of the other. Use different host names, such as `http://fileship.localhost:3000`.
- **A custom domain** has its own login; see [Custom domains](../proxy/custom-domains.md#things-to-know).
- Changing your password, or an admin setting a new one, signs out every other session.
- Sessions do not expire on their own. If you are logged out without any of the above, check **Settings → Sessions** for sessions you don't know and sign them out.

## The login works over HTTP but not over HTTPS (or the other way)

The login cookie is marked `Secure` when Fileship knows the request was HTTPS, from `X-Forwarded-Proto`. A proxy that sends `X-Forwarded-Proto: https` for plain HTTP requests produces a cookie the browser won't send back. Make the header match what the browser used.

## Passkeys don't work

- Passkeys need HTTPS, or `http://localhost`. On plain HTTP the browser does not offer them (**Your browser does not support passkeys**).
- A passkey belongs to the domain it was created on. A passkey made on `files.example.com` does not work on another address of the same server.
- **This passkey isn't registered here** means the passkey was deleted in Fileship, or belongs to another instance.

## Two-factor authentication

- **Invalid two-factor authentication code**: check that the phone's clock is set automatically; codes depend on the time. A code can only be used once.
- Lost your authenticator app? Log in with a passkey if you added one. Admins can't turn two-factor authentication off from the dashboard, and **Forgot your password?** still asks for a code. The server's owner can turn it off in the database (replace `alice` with the username):

    ```sh
    docker compose exec postgres psql -U postgres -c "UPDATE \"User\" SET \"totpEnabled\" = false, \"totpSecret\" = NULL WHERE username = 'alice';"
    ```

- Logins through [single sign-on](../configuration/single-sign-on.md) skip Fileship's two-factor authentication.

## Locked out by Turnstile

A wrong Turnstile secret key, or a domain that is not listed in the Turnstile widget, makes every login fail with **The security check failed, try again**. Remove the saved keys from the database and restart Fileship. With Docker:

```sh
docker compose exec postgres psql -U postgres -c "DELETE FROM \"Setting\" WHERE key IN ('turnstileSiteKey', 'turnstileSecretKey');"
docker compose restart fileship-blue
```

Also remove `NUXT_PUBLIC_TURNSTILE_SITE_KEY` and `NUXT_TURNSTILE_SECRET_KEY` if you set them. Without Docker, run the same `DELETE` with `psql` on your database and restart the service.

## Password login is turned off, log in with single sign-on

**Password and passkey login** is off. Use the provider's button, or, as a super admin, **Log in as an administrator**. See [Single sign-on](../configuration/single-sign-on.md#turning-off-passwords).
