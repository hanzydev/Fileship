<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    Seu próprio servidor ShareX e nuvem de arquivos: um app nativo para Windows, sincronização com o Explorador de Arquivos, pesquisa com IA que roda no seu servidor e uma extensão para navegador.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Baixar para Windows</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Relatar um bug</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Sugerir um recurso</a>
    ·
    <a href="SECURITY.md">Política de segurança</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <b>Português (Brasil)</b> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Ao soltar uma imagem no Fileship, ela é enviada e o link é copiado; depois, a pesquisa inteligente encontra uma foto a partir de uma descrição e mostra a descrição gerada pela IA" width="100%">
</p>

## 📦 Recursos

- Arquivos, pastas aninhadas que você pode compartilhar com outros usuários, notas em Markdown, links curtos e caixas de entrada, com uma lixeira para tudo o que for excluído
- Backups da conta que podem ser baixados e restaurados, criados manualmente ou de forma agendada
- Compartilhamento público com senhas, datas de expiração, limites de visualizações e prévias completas para o Discord e outros apps
- [IA](docs/ai.md) que roda no seu servidor: pesquisa inteligente em mais de 50 idiomas, descrições, OCR e detecção de conteúdo sensível, tudo opcional
- Envios armazenados no disco local, em qualquer serviço compatível com S3, SFTP ou FTP
- Vários usuários com permissões e limites de armazenamento, chaves de acesso, verificação em duas etapas e [login único](docs/configuration.md#single-sign-on) com OpenID Connect
- Atualizações sem tempo de inatividade, com um clique ou agendadas
- Integrações de envio para ShareX, macOS e Linux, o [app para Windows](docs/desktop.md) e um [app instalável para celulares](docs/mobile.md) que recebe o que é compartilhado nos apps Android
- Uma [extensão para navegador](docs/browser-extension.md) que envia arquivos, imagens e capturas de tela e encurta links, [na Chrome Web Store](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh) para Chrome e Edge, e compilada a partir do código-fonte para Firefox
- [Webhooks](docs/webhooks.md) para Discord, Slack e seus próprios serviços
- Inglês, turco, alemão, espanhol, francês, italiano, português do Brasil, russo, azerbaijano, japonês, coreano e chinês simplificado

## 🆚 Fileship comparado às alternativas

|                                                      | Fileship                                  | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| ---------------------------------------------------- | ----------------------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX e outros clientes de envio                    | ✅                                        | ✅                                          | ✅                                                 | ✅                                                  |
| App de desktop para captura e gravação               | ✅ Windows                                | ❌                                          | ❌                                                 | ❌                                                  |
| Seus arquivos no Explorador de Arquivos              | ✅ Fileship Harbor                        | ❌                                          | ❌                                                 | ❌                                                  |
| Extensão para navegador                              | ✅                                        | ❌                                          | ❌                                                 | ✅                                                  |
| Pesquisa com IA, descrições, OCR e conteúdo sensível | ✅ no seu servidor                        | ❌                                          | ❌                                                 | ❌                                                  |
| Links curtos                                         | ✅                                        | ✅                                          | ❌                                                 | ✅                                                  |
| Notas ou trechos de texto                            | ✅ Markdown                               | ✅                                          | 🟡 envios de texto                                 | ✅                                                  |
| Pastas                                               | ✅ aninhadas, compartilhadas com usuários | ✅ aninhadas                                | ❌ tags                                            | 🟡 álbuns sem subpastas                             |
| Senhas, expiração e limites de visualizações         | ✅                                        | ✅                                          | ❌                                                 | 🟡 links de álbum expiram                           |
| Armazenamento                                        | Local, S3, SFTP, FTP                      | Local, S3                                   | Local, S3, FTP, Dropbox, Google Cloud              | Local, S3                                           |
| Verificação em duas etapas e chaves de acesso        | ✅                                        | ✅                                          | ❌                                                 | ❌                                                  |
| Login único                                          | ✅ OIDC                                   | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhooks                                             | ✅ Discord, Slack, JSON assinado          | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| Backups da conta restauráveis e agendados            | ✅                                        | 🟡 exportação                               | 🟡 exportação                                      | ❌                                                  |
| Atualizações pelo painel                             | ✅ sem tempo de inatividade               | ❌                                          | 🟡 com tempo de inatividade                        | ❌                                                  |
| Idiomas                                              | 12                                        | 12                                          | 31, alguns incompletos                             | 1                                                   |
| Licença                                              | MIT                                       | MIT                                         | AGPL-3.0                                           | MIT                                                 |

Comparado com Zipline 4.8, XBackBone 3.8 e Chibisafe 6.7 em outubro de 2026. Algo desatualizado? [Abra uma issue](https://github.com/hanzydev/Fileship/issues/new/choose) e isso será corrigido.

## 📸 Capturas de tela

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Painel do Fileship no computador" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Arquivos do Fileship no celular" width="18%">
</p>

## 🖥️ Fileship para Windows

Capture, grave e envie para o seu Fileship pela área de notificação, no estilo do ShareX, e navegue pelos seus arquivos no Explorador de Arquivos com o Fileship Harbor. Requer Windows 11 de 64 bits, versão 24H2 ou mais recente, e um servidor Fileship 6.0.

**[Baixar a versão mais recente](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [Recursos e configuração](docs/desktop.md)

## 🚀 Primeiros passos

Você precisa do [Docker](https://www.docker.com/) com o [Docker Compose](https://docs.docker.com/compose/), 2 núcleos de CPU e 2 GB de RAM (4 GB ou mais com IA; veja os [requisitos](docs/installation.md#requirements)).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

O Fileship já está disponível em `http://localhost:3000`. Entre com `admin` / `password`.

> [!IMPORTANT]
> Altere a senha logo após o primeiro login (**Configurações → Segurança**).

Vai atualizar da versão 5.x? Siga [Atualizar para a 6.0](docs/upgrading.md). Está vindo do Zipline, XBackBone ou Chibisafe? [Traga suas contas, arquivos e links](docs/migrating.md).

## 📚 Documentação

- [Instalação](docs/installation.md): requisitos, navegadores compatíveis e como servir o Fileship no seu próprio domínio
- [Configuração](docs/configuration.md): opções do servidor, armazenamento, lixeira e variáveis de ambiente
- [Atualizações](docs/updating.md): como as atualizações funcionam e como agendá-las
- [Migrar de outro app](docs/migrating.md): importar um servidor do Zipline, XBackBone ou Chibisafe
- [Recursos de IA](docs/ai.md): perfis, modelos, pesquisa inteligente e conteúdo sensível
- [Fileship para Windows](docs/desktop.md)
- [Fileship no celular](docs/mobile.md): instalar o app e compartilhar a partir do Android
- [Webhooks](docs/webhooks.md): eventos, formatos, entrega e verificação de assinatura
- [Telemetria](docs/telemetry.md): o que é coletado e como desativar

## 🧰 Tecnologias

![Nuxt](https://img.shields.io/static/v1?style=for-the-badge&message=Nuxt&color=222222&logo=Nuxt&logoColor=00DC82&label=)
![Vue.js](https://img.shields.io/static/v1?style=for-the-badge&message=Vue.js&color=222222&logo=Vue.js&logoColor=4FC08D&label=)
![Tailwind CSS](https://img.shields.io/static/v1?style=for-the-badge&message=Tailwind+CSS&color=0B1120&logo=TailwindCSS&logoColor=38BDF8&label=)
![shadcn-vue](https://img.shields.io/static/v1?style=for-the-badge&message=shadcn-vue&color=000000&logo=shadcnui&logoColor=FFFFFF&label=)
![TypeScript](https://img.shields.io/static/v1?style=for-the-badge&message=TypeScript&color=3178C6&logo=TypeScript&logoColor=FFFFFF&label=)
![Prisma](https://img.shields.io/static/v1?style=for-the-badge&message=Prisma&color=2D3748&logo=Prisma&logoColor=FFFFFF&label=)
![PostgreSQL](https://img.shields.io/static/v1?style=for-the-badge&message=PostgreSQL&color=4169E1&logo=PostgreSQL&logoColor=FFFFFF&label=)
![Socket.io](https://img.shields.io/static/v1?style=for-the-badge&message=Socket.io&color=010101&logo=Socket.io&logoColor=FFFFFF&label=)
![Transformers.js](https://img.shields.io/static/v1?style=for-the-badge&message=Transformers.js&color=222222&logo=HuggingFace&logoColor=FFD21E&label=)
![Docker](https://img.shields.io/static/v1?style=for-the-badge&message=Docker&color=2496ED&logo=Docker&logoColor=FFFFFF&label=)
![Caddy](https://img.shields.io/static/v1?style=for-the-badge&message=Caddy&color=1F88C0&logo=Caddy&logoColor=FFFFFF&label=)

## ❤️ Como contribuir

Contribuições são bem-vindas. Leia primeiro as [diretrizes de contribuição](CONTRIBUTING.md). O [AGENTS.md](AGENTS.md) descreve a arquitetura e as convenções do código.

## ⚖️ Licença

[MIT](LICENSE)
