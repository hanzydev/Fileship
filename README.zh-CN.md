<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    属于你自己的 ShareX 服务器和文件云：原生 Windows 应用、文件资源管理器同步、在你的服务器上运行的 AI 搜索，以及浏览器扩展。
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">下载 Windows 版</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">报告问题</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">建议新功能</a>
    ·
    <a href="SECURITY.md">安全策略</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <b>简体中文</b>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="将图片拖到 Fileship 上即可上传并复制链接，随后智能搜索根据文字描述找到一张照片，并显示它的 AI 图片描述" width="100%">
</p>

## 📦 功能

- 文件、可与其他用户共享的多级文件夹、Markdown 笔记、短链接和收件箱，删除的内容会先进入回收站
- 可下载、可还原的账户备份，支持手动创建或定时创建
- 公开分享，支持密码、过期时间和浏览次数限制，并在 Discord 等应用中显示丰富的链接预览
- 在你的服务器上运行的 [AI](docs/ai.md)：支持 50 多种语言的智能搜索、图片描述、OCR 和敏感内容检测，全部可选
- 上传的文件可存放在本地磁盘、任意兼容 S3 的服务、SFTP 或 FTP
- 多用户，支持权限和存储限制、通行密钥、双重验证，以及基于 OpenID Connect 的[单点登录](docs/configuration.md#single-sign-on)
- 零停机更新，一键完成或按计划自动进行
- ShareX、macOS 和 Linux 上传集成、[Windows 应用](docs/desktop.md)，以及可接收 Android 应用分享的[手机可安装应用](docs/mobile.md)
- [浏览器扩展](docs/browser-extension.md)，可上传文件、图片和截图并生成短链接。Chrome 和 Edge 可从 [Chrome 应用商店](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh)安装，Firefox 可从 [Firefox 附加组件](https://addons.mozilla.org/firefox/addon/fileship/)安装
- 可推送到 Discord、Slack 和你自己的服务的 [Webhook](docs/webhooks.md)
- 英语、土耳其语、德语、西班牙语、法语、意大利语、巴西葡萄牙语、俄语、阿塞拜疆语、日语、韩语和简体中文

## 🆚 与同类项目对比

|                                   | Fileship                     | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| --------------------------------- | ---------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX 及其他上传工具             | ✅                           | ✅                                          | ✅                                                 | ✅                                                  |
| 用于截图和录制的桌面应用          | ✅ Windows                   | ❌                                          | ❌                                                 | ❌                                                  |
| 在文件资源管理器中访问文件        | ✅ Fileship Harbor           | ❌                                          | ❌                                                 | ❌                                                  |
| 浏览器扩展                        | ✅                           | ❌                                          | ❌                                                 | ✅                                                  |
| AI 搜索、图片描述、OCR 和敏感内容 | ✅ 在你的服务器上运行        | ❌                                          | ❌                                                 | ❌                                                  |
| 短链接                            | ✅                           | ✅                                          | ❌                                                 | ✅                                                  |
| 笔记或文本片段                    | ✅ Markdown                  | ✅                                          | 🟡 文本上传                                        | ✅                                                  |
| 文件夹                            | ✅ 多级，可与用户共享        | ✅ 多级                                     | ❌ 标签                                            | 🟡 单层相册                                         |
| 密码、过期时间和浏览次数限制      | ✅                           | ✅                                          | ❌                                                 | 🟡 相册链接可过期                                   |
| 存储                              | 本地、S3、SFTP、FTP          | 本地、S3                                    | 本地、S3、FTP、Dropbox、Google Cloud               | 本地、S3                                            |
| 双重验证和通行密钥                | ✅                           | ✅                                          | ❌                                                 | ❌                                                  |
| 单点登录                          | ✅ OIDC                      | ✅ OAuth、OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhook                           | ✅ Discord、Slack、签名 JSON | ✅ Discord、HTTP                            | ❌                                                 | ❌                                                  |
| 可还原的账户备份，支持定时        | ✅                           | 🟡 导出                                     | 🟡 导出                                            | ❌                                                  |
| 在控制台中更新                    | ✅ 零停机                    | ❌                                          | 🟡 需要停机                                        | ❌                                                  |
| 语言                              | 12                           | 12                                          | 31，部分未译完                                     | 1                                                   |
| 许可证                            | MIT                          | MIT                                         | AGPL-3.0                                           | MIT                                                 |

以上对比基于 2026 年 10 月的 Zipline 4.8、XBackBone 3.8 和 Chibisafe 6.7。发现内容过时了？欢迎[提交 issue](https://github.com/hanzydev/Fileship/issues/new/choose)，我们会及时修正。

## 📸 截图

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="桌面端的 Fileship 控制台" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="手机上的 Fileship 文件列表" width="18%">
</p>

## 🖥️ Fileship Windows 版

在通知区域中以 ShareX 的方式截图、录制并上传到你的 Fileship，还能通过 Fileship Harbor 在文件资源管理器中浏览你的文件。需要 64 位 Windows 11 24H2 或更高版本，以及 Fileship 6.0 服务器。

**[下载最新版本](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [功能与设置](docs/desktop.md)

## 🚀 快速开始

你需要安装 [Docker](https://www.docker.com/) 和 [Docker Compose](https://docs.docker.com/compose/)，并准备 2 个 CPU 核心和 2 GB 内存（启用 AI 时需要 4 GB 或更多，详见[系统要求](docs/installation.md#requirements)）。

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

现在可以通过 `http://localhost:3000` 访问 Fileship。使用 `admin` / `password` 登录。

> [!IMPORTANT]
> 首次登录后请立即修改密码（**设置 → 安全**）。

从 5.x 升级？请参阅[升级到 6.0](docs/upgrading.md)。从 Zipline、XBackBone 或 Chibisafe 迁移？[把账户、文件和链接一起迁移过来](docs/migrating.md)。

## 📚 文档

- [安装](docs/installation.md)：系统要求、浏览器支持，以及在自己的域名上部署 Fileship
- [配置](docs/configuration.md)：服务器设置、存储、回收站和环境变量
- [更新](docs/updating.md)：更新的工作方式以及如何定时更新
- [从其他应用迁移](docs/migrating.md)：导入 Zipline、XBackBone 或 Chibisafe 服务器
- [AI 功能](docs/ai.md)：预设、模型、智能搜索和敏感内容
- [Fileship Windows 版](docs/desktop.md)
- [在手机上使用 Fileship](docs/mobile.md)：安装应用以及从 Android 分享
- [Webhook](docs/webhooks.md)：事件、格式、投递和签名验证
- [遥测](docs/telemetry.md)：收集哪些数据以及如何关闭

## 🧰 技术栈

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

## ❤️ 参与贡献

欢迎贡献代码。请先阅读[贡献指南](CONTRIBUTING.md)。[AGENTS.md](AGENTS.md) 介绍了代码库的架构和约定。

## ⚖️ 许可证

[MIT](LICENSE)
