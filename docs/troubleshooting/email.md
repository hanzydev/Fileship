---
title: Email problems
description: Fix "Couldn't send the test email" and missing emails in Fileship, by SMTP error.
---

# Email problems

## Couldn't send the test email: …

**Send test email** shows the SMTP server's answer after the colon:

| The error contains                                                  | Cause and fix                                                                                                                |
| ------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------------------------------------- |
| `ECONNREFUSED`                                                      | Nothing listens on that host and port. Check **SMTP server** and **Port**.                                                   |
| `ETIMEDOUT`, `Connection timeout`, `Greeting never received`        | A firewall blocks the port, or the encryption does not match the port. Many hosting providers block port 25; use 587 or 465. |
| `ENOTFOUND`, `EAI_AGAIN`                                            | The host name does not resolve. Check for typos; inside Docker, `localhost` is the container itself.                         |
| `wrong version number`, `SSL routines`                              | **TLS** on a STARTTLS port or the other way round. Use **STARTTLS** with 587 and **TLS** with 465.                           |
| `self-signed certificate`, `unable to verify the first certificate` | The server's certificate is not trusted. Use the provider's real host name, or a trusted certificate.                        |
| `535`, `Invalid login`, `Authentication failed`                     | Wrong username or password. Gmail needs an app password; Microsoft 365 needs SMTP AUTH enabled for the mailbox.              |
| `530`, `Must issue a STARTTLS command first`                        | The server requires encryption. Choose **STARTTLS**.                                                                         |
| `553`, `554`, `Sender address rejected`, `not owned by user`        | The **Sender address** is not allowed for this account. Use an address or domain your provider verified.                     |

## Email isn't set up on this instance

An email feature was used while email is off. Set the **Dashboard address**, the **SMTP server** and the **Sender address** (see [Email](../configuration/email.md)).

## Couldn't send the email. Try again later.

Sending a real email (a confirmation link, a reset link, an invite) failed. The server output has the SMTP error; use **Send test email** to see it in the dashboard.

## Emails arrive in spam

Set up SPF, DKIM and DMARC for the sender's domain with your email provider, and send from a domain you own. Fileship only hands the email to your SMTP server.

## Links in emails point to the wrong address

They use the **Dashboard address**, never the address a request came from. Set it to your public `https://` address.

## Password reset emails never arrive

The reset page answers the same way whether or not the account exists, so check that the account has a **confirmed** email address (the **Users** page shows it). Reset requests are limited to 3 per account per hour.

## This link is invalid or has expired

Confirmation links work for 24 hours, reset links for one hour and invites for 7 days, each once. A newer link replaces the older one. Ask for a new link.
