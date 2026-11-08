---
title: Single sign-on
description: Log in to Fileship through an OpenID Connect provider such as Keycloak, Authentik, Authelia, Google, Microsoft Entra ID, Pocket ID or Zitadel.
---

# Single sign-on

Fileship can log people in through an OpenID Connect provider. Set it up in **Administration → Instance → Single sign-on**, or give the defaults with the `NUXT_OIDC_*` variables and `NUXT_PASSWORD_LOGIN_ENABLED` (see [Environment variables](environment.md#registration-and-single-sign-on)). Fileship supports one provider per instance, and OpenID Connect only (no plain OAuth 2.0, SAML or LDAP).

## Set it up

1. Set the **Dashboard address** in **General**. The redirect URI is built from it, for example `https://files.example.com/api/auth/oidc/callback`, never from the address a request came from. The section shows it under **Redirect URI**.
2. At your provider, create an OpenID Connect client (often called an application) for Fileship with the authorization code flow, that redirect URI and the `openid`, `email` and `profile` scopes. A confidential client with a secret is usual. A public client works too, because Fileship always uses PKCE.
3. Turn on **Enable single sign-on** and enter the **Issuer URL**, the **Client ID** and the **Client secret** (empty for a public client). **Test** checks that Fileship can read the provider's discovery document. The secret is encrypted with `data/secret.key` and never shown again; leave the field empty to keep it.
4. Optionally set the **Provider name** for the login button, for example `Authentik` for **Continue with Authentik**.

The issuer must be exactly what the provider publishes in its discovery document (`<issuer>/.well-known/openid-configuration`), including a trailing slash if it has one. Fileship can reach `http://` issuers too, for a provider on your own network.

Fileship authenticates to the token endpoint with `client_secret_basic` when the provider supports it, otherwise `client_secret_post`. It reads the claims from the ID token and the userinfo endpoint.

## Providers

| Provider                                  | Issuer URL                                           | Tested           |
| ----------------------------------------- | ---------------------------------------------------- | ---------------- |
| [Keycloak](#keycloak)                     | `https://auth.example.com/realms/<realm>`            | yes, Keycloak 26 |
| [Google](#google)                         | `https://accounts.google.com`                        | yes              |
| [Authentik](#authentik)                   | `https://auth.example.com/application/o/<slug>/`     | no               |
| [Authelia](#authelia)                     | `https://auth.example.com`                           | no               |
| [Microsoft Entra ID](#microsoft-entra-id) | `https://login.microsoftonline.com/<tenant id>/v2.0` | no               |
| [Pocket ID](#pocket-id)                   | `https://id.example.com`                             | no               |
| [Zitadel](#zitadel)                       | `https://<instance>.zitadel.cloud`                   | no               |

"No" means the steps follow the provider's documentation and the standard flow Fileship uses, but nobody has logged in through that provider yet. Reports are welcome.

### Keycloak

1. In your realm, open **Clients → Create client**, type **OpenID Connect**, a client ID such as `fileship`.
2. Turn on **Client authentication** (confidential) and keep **Standard flow** on.
3. Add the redirect URI to **Valid redirect URIs** and your dashboard address to **Web origins**.
4. Copy the secret from the **Credentials** tab.
5. Issuer: `https://auth.example.com/realms/<realm>`.

Users need an email with **Email verified** on to be matched to existing Fileship accounts by email.

### Google

1. In the [Google Cloud console](https://console.cloud.google.com/apis/credentials), configure the OAuth consent screen, then create an **OAuth client ID** of type **Web application**.
2. Add the redirect URI under **Authorized redirect URIs**.
3. Issuer: `https://accounts.google.com`.

Logging in with Google needs Fileship 6.3.1 or newer; 6.3.0 failed with `invalid_client` (see [Single sign-on problems](../troubleshooting/single-sign-on.md#google-says-invalid_client)). Google accounts have no username, so new accounts are named after the email address.

### Authentik

1. Create an **OAuth2/OpenID Provider** with **Client type** confidential, the redirect URI (strict) and the default scopes `openid`, `email` and `profile`.
2. Create an **Application** for it; its slug is part of the issuer.
3. Issuer: `https://auth.example.com/application/o/<slug>/`, with the trailing slash.

### Authelia

Add a client to Authelia's configuration:

```yaml
identity_providers:
    oidc:
        clients:
            - client_id: fileship
              client_name: Fileship
              client_secret: '$pbkdf2-sha512$...' # the hashed secret
              authorization_policy: two_factor
              redirect_uris:
                  - https://files.example.com/api/auth/oidc/callback
              scopes: [openid, email, profile]
              require_pkce: true
              pkce_challenge_method: S256
              token_endpoint_auth_method: client_secret_basic
```

Issuer: your Authelia address, such as `https://auth.example.com`. Recent Authelia versions put the email and username in the userinfo response instead of the ID token; Fileship reads both.

### Microsoft Entra ID

1. In **App registrations → New registration**, add the redirect URI as a **Web** platform.
2. Create a secret in **Certificates & secrets**.
3. Issuer: `https://login.microsoftonline.com/<tenant id>/v2.0`.

Entra ID does not send `email_verified`, so accounts are not matched by email: turn on **Create accounts**, or have people link their account in **Settings → Security**.

### Pocket ID

Create an **OIDC client** with the redirect URI as callback URL. Issuer: your Pocket ID address. Pocket ID logs in with passkeys, so Fileship's own passkeys are not needed for those users.

### Zitadel

Create a **Web** application with the **Code** flow and the redirect URI, and authentication method **Basic**. Issuer: your instance address, such as `https://<instance>.zitadel.cloud`.

## How accounts are matched

When someone logs in through the provider, Fileship looks for their account in this order:

1. The account linked to that provider account (its issuer and subject).
2. The account whose confirmed email address is the provider account's email, but only when the provider says the email is verified (`email_verified`). Fileship links the two and logs them in. If that account is already linked to another account at the same provider, the login is refused.
3. With **Create accounts** on, a new account with the registration permissions and storage limit from **Registration** (self-registration itself can stay off). Its username comes from `preferred_username`, the nickname, the email address or the name, with a number added when it is taken, and a verified email becomes its confirmed address. Such an account has no password until its owner sets one in **Settings → Security**.

Otherwise the login page says that no account is linked.

## Linking an account

People can link their account themselves in **Settings → Security → Single sign-on**. Linking and unlinking ask them to confirm it's them first, and Fileship refuses to unlink while the account could not log in another way: without a password or passkey, or while password login is off.

Accounts that only log in through the provider confirm sensitive actions by logging in at the provider again in a small window, so allow pop-ups for the dashboard address. Linking and confirming ask the provider to re-authenticate (`max_age=0`); a provider that ignores it may accept its current session instead.

## Turning off passwords

Turn off **Password and passkey login** to have everyone log in through the provider. Super admins can still log in with their password or a passkey through **Log in as an administrator**, so a broken provider never locks you out. The switch only takes effect while single sign-on is set up. Some things to know:

- Logins through the provider skip Fileship's two-factor authentication. Require it at the provider instead.
- While password login is off, accounts created with an invite or through self-registration can only log in again through the provider. They are matched by their confirmed email, or can link the provider right after signing up; **Create accounts** is usually the simpler choice.
- Fileship does not ask the provider again after someone has logged in, so disabling them there does not end their open sessions. Setting a new password for them on the **Users** page does.

## Good to know

- Single sign-on only works at the dashboard address. A login started at another address of the same server is sent there first, and linking only works there.
- The Windows app logs in through the same login page, and linking an account and confirming actions with the provider work in its **Dashboard** window too.
- Changing the issuer URL keeps the old links, which then match nothing. People link again, or are matched by their confirmed email.
- Linked provider accounts are not part of account backups.
- Logins, callbacks and tests are rate limited per IP address.

Problems? See [Single sign-on problems](../troubleshooting/single-sign-on.md).
