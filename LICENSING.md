# Licensing

Fileship is open source. Starting with version 6.6.0, the Fileship server is licensed under the [GNU Affero General Public License v3.0](LICENSE) (AGPL-3.0-only) with one additional term, and the apps and shared packages stay under the MIT License.

This page is a summary for convenience. The license texts decide. It is not legal advice.

## What is licensed how

| Part                                                                                                                                                                                         | License                                                                 |
| -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------------------------------- |
| Everything not listed below: the dashboard and its API, the updater agent, the telemetry panel, the landing and docs site, `packages/ai`, `packages/pdf`, the Docker files, scripts and docs | AGPL-3.0-only with the [attribution term](#additional-term-attribution) |
| `apps/desktop` (Fileship for Windows)                                                                                                                                                        | [MIT](apps/desktop/LICENSE)                                             |
| `apps/extension` (the browser extension)                                                                                                                                                     | [MIT](apps/extension/LICENSE)                                           |
| `packages/ui`                                                                                                                                                                                | [MIT](packages/ui/LICENSE)                                              |
| `packages/telemetry`                                                                                                                                                                         | [MIT](packages/telemetry/LICENSE)                                       |
| `packages/eslint-config`                                                                                                                                                                     | [MIT](packages/eslint-config/LICENSE)                                   |

A folder with its own `LICENSE` file is licensed by that file. Everything else is licensed by the [`LICENSE`](LICENSE) file at the root of the repository, together with the additional term below.

## Additional term: attribution

Under section 7(b) of the AGPL-3.0, the following term applies to the parts of Fileship licensed under the AGPL-3.0. It is part of the license, and it travels with every copy and every modified version.

> If you convey the Program or a work based on it, or let users interact with it remotely through a computer network, you must keep a visible "Powered by Fileship" notice that links to https://fileship.app on the login page of its user interface. If a modified version has no login page, the notice must be shown in an equally visible place that every user can reach, such as the footer or an "About" page. You may change the notice's styling and translate it, but you may not remove it, hide it or make it unreadable.

This is the only term added to the AGPL-3.0. Everything else about the AGPL-3.0 applies unchanged.

## What this means in practice

You can:

- Self-host Fileship for yourself, your family, your community or your company, for free.
- Change it however you like.
- Use it commercially, including offering Fileship as a paid service to others.

When you do, you must:

- Keep the "Powered by Fileship" notice on the login page.
- Keep the copyright and license notices.
- If you change Fileship and let people use your changed version over a network, offer those people the complete source code of your version under the same license. Running an unmodified Fileship needs nothing more than the notice, because its source is already public.

You may not call your service or product "Fileship", or use the Fileship name or logo in a way that suggests it is the official Fileship or endorsed by it. See the [trademark policy](TRADEMARKS.md). Saying that it is "based on Fileship" or "powered by Fileship" is fine.

## Commercial licenses

If the AGPL-3.0 does not work for you, for example because you want to ship Fileship inside a closed-source product or remove the attribution, a commercial license is available. Write to contact@hanzy.dev.

## Contributing

Contributions are accepted under the [Contributor License Agreement](CLA.md). You sign it once, with a comment on your first pull request. It lets the project offer the commercial license above and change its license in the future, while your contribution stays available under the open-source license it was released with.

## Earlier versions

Fileship 6.5.0 and every earlier version were released under the MIT License, and they stay under it. Code that was first published under the MIT License before 6.6.0 can still be used under that license, from those releases. Its notice is kept here, as the MIT License requires:

```text
MIT License

Copyright (c) 2024-present Hànzy and Fileship contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Third-party software

Fileship uses third-party packages and AI models under their own licenses, among them the AI models SigLIP (Apache-2.0), Florence-2 (MIT), paraphrase-multilingual-MiniLM (Apache-2.0), the OpenAI Privacy Filter (Apache-2.0) and PaddleOCR (Apache-2.0). The Docker image of the server includes FFmpeg and FFprobe builds that are licensed under the GNU GPL v3; FFmpeg runs as a separate program, and its source code is available at https://ffmpeg.org.
