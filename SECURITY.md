# Security Policy

## Supported versions

| Version | Supported                                  |
| ------- | ------------------------------------------ |
| 6.x     | :white_check_mark:                         |
| 5.4.x   | Security fixes only, until 6.1 is released |
| < 5.4   | :x:                                        |

## Reporting a vulnerability

Please **do not** report security vulnerabilities through public GitHub issues, discussions or pull requests.

Report them privately through [GitHub Security Advisories](https://github.com/hanzydev/Fileship/security/advisories/new) (**Security → Report a vulnerability**).

Please include:

- a description of the vulnerability and its potential impact
- the affected version and configuration
- steps or a proof of concept to reproduce it
- any suggested fix or mitigation (optional)

### What to expect

- You will receive an acknowledgement as soon as possible, usually within a few days.
- The report is investigated and, once confirmed, a fix is prepared and released.
- The advisory is published after a fixed version is available. You will be credited unless you prefer to stay anonymous.

## Hardening your instance

- Change the default `admin` password right after installation.
- Keep the `SECRET_TOKEN` in `.env` private; it authorizes updates.
- Serve Fileship over HTTPS behind a reverse proxy.
- Keep self-registration turned off unless you need it, and enable Cloudflare Turnstile if the login page is public.
- Enable two-factor authentication (an authenticator app or a passkey) for admin accounts.
- Keep Fileship up to date. Scheduled updates are enabled by default.
