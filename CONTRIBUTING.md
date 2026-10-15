# Contributing to Fileship

Thank you for your interest in contributing. Bug reports, feature ideas, documentation fixes and pull requests are all welcome.

This project follows a [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you agree to uphold it.

## Reporting bugs and requesting features

- Search the [existing issues](https://github.com/hanzydev/Fileship/issues) before opening a new one.
- Use the [bug report](https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml) or [feature request](https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml) template and include as much detail as you can.
- **Do not report security vulnerabilities in public issues.** Follow the [security policy](SECURITY.md) instead.

For larger changes, please open an issue first so the approach can be discussed before you invest time in it.

## Development setup

### Prerequisites

- [Node.js](https://nodejs.org/) 24
- [pnpm](https://pnpm.io/) 11 (the repository enforces pnpm)
- [Docker](https://www.docker.com/), or any PostgreSQL 16 server

### Getting started

1. Fork and clone the repository.

2. Start a PostgreSQL database:

    ```sh
    docker run -d --name fileship-postgres -p 5432:5432 \
        -e POSTGRES_USER=postgres -e POSTGRES_PASSWORD=postgres -e POSTGRES_DB=fileship \
        postgres:16
    ```

3. Create `apps/dashboard/.env` from [`apps/dashboard/.env.example`](apps/dashboard/.env.example) and set `DATABASE_URL`:

    ```sh
    DATABASE_URL=postgresql://postgres:postgres@localhost:5432/fileship
    ```

4. Install the dependencies and start the dashboard:

    ```sh
    pnpm install
    pnpm dev
    ```

    The dashboard generates the Prisma client and applies the migrations before it starts. Open `http://localhost:3000` and sign in with `admin` / `password`.

To work on the telemetry panel, create `apps/telemetry/.env` from [`apps/telemetry/.env.example`](apps/telemetry/.env.example) with its own database and a password, then run `pnpm --filter telemetry dev` (port 3002). Set `NUXT_TELEMETRY_URL=http://localhost:3002` in the dashboard to send it reports.

To work on AI features without loading the models, set `NUXT_PUBLIC_AI_ENABLED=false`, or turn AI off in **Administration → Instance**.

### Project structure

Fileship is a pnpm and Turborepo monorepo:

- `apps/dashboard`: the Nuxt application (UI and API)
- `apps/telemetry`: the panel that receives anonymous telemetry reports
- `apps/updater-agent`: the service that performs zero-downtime updates
- `packages/ui`: the shared Nuxt layer with the design system, used by both Nuxt apps
- `packages/telemetry`: the telemetry report schema and client
- `packages/eslint-config`: the shared ESLint configuration

[AGENTS.md](AGENTS.md) documents the architecture, the shared building blocks and the code conventions in detail. Please read it before making larger changes.

## Making changes

### Code style

- Formatting is handled by Prettier (4 spaces, single quotes, 100 columns), and linting by ESLint.
- The dashboard UI is built with [shadcn-vue](https://www.shadcn-vue.com/). Use its components as documented, and don't restyle the files in `packages/ui/app/components/ui`; adjust styles where a component is used instead.
- Reuse the existing composables and utilities (lists, confirmations, verification, settings) rather than adding new versions of them.

### Database changes

Create migrations with `pnpm --filter dashboard db:migrate` and commit the generated files.

Migrations must stay **backward compatible**. During an update, the old and new versions briefly run against the same database, so add new columns and tables first and remove or rename old ones in a later release.

### Commits

Use [Conventional Commits](https://www.conventionalcommits.org/en/v1.0.0/) (for example `feat(ui): ...`, `fix(api): ...`, `docs: ...`). They keep the history readable, and `CHANGELOG.md` is written from them when a version is released. Mark breaking changes with `!` and a `BREAKING CHANGE:` footer.

## Submitting a pull request

Before opening a pull request, make sure that:

- `pnpm lint:types` reports no errors
- `pnpm lint` passes
- `pnpm build` succeeds
- you have tested your change locally, including on a narrow screen for UI changes
- documentation is updated where needed (`README.md`, `AGENTS.md`, `apps/dashboard/.env.example`, `apps/telemetry/.env.example`)

Keep pull requests focused: unrelated changes are easier to review as separate pull requests. Draft pull requests are welcome if you would like early feedback.

## Resources

- [How to Contribute to Open Source](https://opensource.guide/how-to-contribute/)
- [About pull requests](https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/proposing-changes-to-your-work-with-pull-requests/about-pull-requests)
