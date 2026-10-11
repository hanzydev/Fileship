<!---

*** PLEASE READ BEFORE OPENING A PULL REQUEST ***

- The title must follow Conventional Commits (https://conventionalcommits.org), for example "fix(ui): ...".
- For a bug fix, link an issue with a reproduction, or explain what is being fixed and why this fix is the right one.
- For a new feature, make sure it has been discussed in an issue first. Keep the scope small and split large changes into several pull requests.
- See CONTRIBUTING.md for the development setup and the checks to run.

Thank you for your contribution ❤️
-->

### 🔗 Linked issue

<!-- Mention the related issue, for example "Resolves #123". -->

### ❓ Type of change

<!-- Put an `x` in every box that applies. -->

- [ ] 🐞 Bug fix (a non-breaking change that fixes an issue)
- [ ] 👌 Enhancement (an improvement to existing functionality, such as performance)
- [ ] ✨ New feature (a non-breaking change that adds functionality)
- [ ] 🧹 Chore (build process, tooling or dependency updates)
- [ ] 🛠️ Refactor (a change that neither fixes a bug nor adds a feature)
- [ ] 📖 Documentation
- [ ] ⚠️ Breaking change (a change that requires users to update their setup or data)

### 📚 Description

<!-- What does this change do, and why is it needed? -->
<!-- For UI changes, add before and after screenshots, including a narrow screen if relevant. -->

### 📝 Checklist

<!-- Put an `x` in every box that applies. If you are unsure about any of them, just ask. -->

- [ ] I have linked an issue or discussion.
- [ ] `pnpm --filter dashboard lint:types` reports no errors and `pnpm lint` passes.
- [ ] I have tested the change locally.
- [ ] Database migrations, if any, are backward compatible.
- [ ] I have updated the documentation (`README.md`, `AGENTS.md`, `.env.example`) where needed.
