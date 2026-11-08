---
title: Email
description: Set up SMTP so Fileship can send email confirmation and password reset links, invites and storage warnings.
---

# Email

Fileship can send email: links to confirm an email address or reset a password, invites and storage warnings. Email is optional. Until it is set up, every email feature stays hidden.

## Set it up

Open **Administration → Instance**, or give the defaults with `NUXT_SITE_URL` and the `NUXT_SMTP_*` [variables](environment.md#email):

- **Dashboard address** (in **General**): the address people open Fileship at, for example `https://files.example.com`. Links in emails always use it, never the address a request came from.
- **SMTP server**, **Port** and **Encryption** (in **Email**): **STARTTLS** (usually port 587), **TLS** (usually port 465) or **None**.
- **Username** and **Password**: leave them empty if the server accepts mail without signing in. The password is encrypted with `data/secret.key` and never shown again; leave the field empty to keep it.
- **Sender address**: emails come from this address, with the site name as the sender name. Use an address your SMTP server is allowed to send from.

**Send test email** under **Test email** sends a message to the address you type, using the values in the form, so you can try them before saving. If the server refuses, the error says why (see [Email problems](../troubleshooting/email.md)). A test that uses the stored password instead of a newly typed one asks you to confirm it's you first.

Email features appear once the dashboard address, the SMTP server and the sender address are set. Emails are written in the recipient's language, as HTML and plain text.

### Common SMTP providers

| Provider                 | Server                              | Port and encryption | Username                                                                                |
| ------------------------ | ----------------------------------- | ------------------- | --------------------------------------------------------------------------------------- |
| Gmail / Google Workspace | `smtp.gmail.com`                    | 587, STARTTLS       | your address, with an [app password](https://support.google.com/accounts/answer/185833) |
| Microsoft 365            | `smtp.office365.com`                | 587, STARTTLS       | your address (SMTP AUTH must be enabled for the mailbox)                                |
| Amazon SES               | `email-smtp.<region>.amazonaws.com` | 587, STARTTLS       | SMTP credentials from the SES console                                                   |
| Resend                   | `smtp.resend.com`                   | 465, TLS            | `resend`, password = API key                                                            |
| Mailgun                  | `smtp.mailgun.org`                  | 587, STARTTLS       | the domain's SMTP login                                                                 |

These values come from each provider's documentation. Fileship was tested with a local SMTP server; sending through real providers is not tested yet.

## Email addresses

Once email works, people can add an email address in **Settings → Profile → Email**. Fileship sends a confirmation link that is valid for 24 hours and only uses the address after it is confirmed. An address can belong to one account only. Admins see each user's address, and whether it is confirmed, on the **Users** page.

## Password reset

**Forgot your password?** on the login page sends a reset link to the account's confirmed address. The link works once, for one hour, and resetting the password signs the account out everywhere. Accounts that use an authenticator app must enter a code too, so access to the mailbox alone is never enough. The page answers the same way whether or not an account matches, so it never reveals which accounts exist. Without a confirmed address, an admin can still set a new password on the **Users** page.

## Invites

Admins can invite people from the **Users** page: **Invite** sends a link to create an account with the permissions and storage limit you choose, in the language you pick. The link works once, for seven days, even while self-registration is off, and the invited address becomes the account's confirmed email. Pending invites are listed above the users and can be withdrawn. See [Registration and invites](registration.md).

## Storage warnings

When someone with a storage limit and a confirmed address passes 90% of it, Fileship emails them once, with a link to their storage page. Files in the trash count too. The warning is sent again only after their usage has dropped below 90%.
