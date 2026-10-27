<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    自分だけの ShareX サーバー兼ファイルクラウド。ネイティブの Windows アプリ、エクスプローラーとの同期、自分のサーバーで動く AI 検索、ブラウザー拡張機能を備えています。
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Windows 版をダウンロード</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">バグを報告</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">機能をリクエスト</a>
    ·
    <a href="SECURITY.md">セキュリティポリシー</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <b>日本語</b> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="画像を Fileship にドロップするとアップロードされてリンクがコピーされ、スマート検索が文章から写真を見つけて AI による説明文を表示する様子" width="100%">
</p>

## 📦 機能

- ファイル、他のユーザーと共有できる階層フォルダー、Markdown ノート、短縮リンク、受信ボックス。削除したものはゴミ箱に入ります
- ダウンロードや復元ができるアカウントのバックアップ。手動でもスケジュールでも作成できます
- パスワード、有効期限、閲覧数の上限を設定できる公開共有と、Discord などのアプリで表示されるリッチなリンクのプレビュー
- 自分のサーバーで動く [AI](docs/ai.md)：50 以上の言語に対応したスマート検索、説明文、OCR、センシティブなコンテンツの検出。すべて任意です
- アップロード先はローカルディスク、S3 互換サービス、SFTP、FTP から選べます
- 権限と容量制限を設定できる複数ユーザー、パスキー、2 段階認証、OpenID Connect による[シングルサインオン](docs/configuration.md#single-sign-on)
- ダウンタイムなしのアップデート。ワンクリックでもスケジュールでも実行できます
- ShareX、macOS、Linux 向けのアップロード連携、[Windows アプリ](docs/desktop.md)、Android アプリから共有できる[スマートフォン用のインストール可能なアプリ](docs/mobile.md)
- ファイル、画像、スクリーンショットのアップロードとリンクの短縮ができる[ブラウザー拡張機能](docs/browser-extension.md)。Chrome と Edge 向けは [Chrome ウェブストア](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh)で公開しており、Firefox 向けはソースからビルドします
- Discord、Slack、独自のサービスに送れる [Webhook](docs/webhooks.md)
- 英語、トルコ語、ドイツ語、スペイン語、フランス語、イタリア語、ブラジルポルトガル語、ロシア語、アゼルバイジャン語、日本語、韓国語、簡体字中国語

## 🆚 他のツールとの比較

|                                                | Fileship                         | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| ---------------------------------------------- | -------------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX などのアップローダー                    | ✅                               | ✅                                          | ✅                                                 | ✅                                                  |
| キャプチャと録画ができるデスクトップアプリ     | ✅ Windows                       | ❌                                          | ❌                                                 | ❌                                                  |
| エクスプローラーでファイルを表示               | ✅ Fileship Harbor               | ❌                                          | ❌                                                 | ❌                                                  |
| ブラウザー拡張機能                             | ✅                               | ❌                                          | ❌                                                 | ✅                                                  |
| AI 検索、説明文、OCR、センシティブなコンテンツ | ✅ 自分のサーバーで動作          | ❌                                          | ❌                                                 | ❌                                                  |
| 短縮リンク                                     | ✅                               | ✅                                          | ❌                                                 | ✅                                                  |
| ノートやテキスト共有                           | ✅ Markdown                      | ✅                                          | 🟡 テキストのアップロード                          | ✅                                                  |
| フォルダー                                     | ✅ 階層あり、ユーザーと共有      | ✅ 階層あり                                 | ❌ タグ                                            | 🟡 階層のないアルバム                               |
| パスワード、有効期限、閲覧数の上限             | ✅                               | ✅                                          | ❌                                                 | 🟡 アルバムのリンクに有効期限                       |
| ストレージ                                     | ローカル、S3、SFTP、FTP          | ローカル、S3                                | ローカル、S3、FTP、Dropbox、Google Cloud           | ローカル、S3                                        |
| 2 段階認証とパスキー                           | ✅                               | ✅                                          | ❌                                                 | ❌                                                  |
| シングルサインオン                             | ✅ OIDC                          | ✅ OAuth、OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhook                                        | ✅ Discord、Slack、署名付き JSON | ✅ Discord、HTTP                            | ❌                                                 | ❌                                                  |
| 復元できるアカウントのバックアップ（定期実行） | ✅                               | 🟡 エクスポート                             | 🟡 エクスポート                                    | ❌                                                  |
| ダッシュボードからのアップデート               | ✅ ダウンタイムなし              | ❌                                          | 🟡 ダウンタイムあり                                | ❌                                                  |
| 対応言語                                       | 12                               | 12                                          | 31（一部は翻訳途中）                               | 1                                                   |
| ライセンス                                     | MIT                              | MIT                                         | AGPL-3.0                                           | MIT                                                 |

2026 年 10 月時点の Zipline 4.8、XBackBone 3.8、Chibisafe 6.7 と比較しています。情報が古くなっていたら [Issue を作成](https://github.com/hanzydev/Fileship/issues/new/choose)してください。修正します。

## 📸 スクリーンショット

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="デスクトップで表示した Fileship のダッシュボード" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="スマートフォンで表示した Fileship のファイル一覧" width="18%">
</p>

## 🖥️ Fileship for Windows

通知領域から ShareX のようにキャプチャ、録画、Fileship へのアップロードができ、Fileship Harbor を使えばエクスプローラーでファイルを閲覧できます。64 ビット版 Windows 11 バージョン 24H2 以降と、Fileship 6.0 のサーバーが必要です。

**[最新版をダウンロード](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [機能とセットアップ](docs/desktop.md)

## 🚀 はじめに

[Docker](https://www.docker.com/) と [Docker Compose](https://docs.docker.com/compose/)、2 コアの CPU、2 GB の RAM が必要です（AI を使う場合は 4 GB 以上。[動作要件](docs/installation.md#requirements)を参照してください）。

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

これで `http://localhost:3000` から Fileship を使えます。`admin` / `password` でログインしてください。

> [!IMPORTANT]
> 初回ログイン後、すぐにパスワードを変更してください（**設定 → セキュリティ**）。

5.x からアップグレードする場合は、[6.0 へのアップグレード](docs/upgrading.md)の手順に従ってください。Zipline、XBackBone、Chibisafe から移る場合は、[アカウント、ファイル、リンクをそのまま移行できます](docs/migrating.md)。

## 📚 ドキュメント

- [インストール](docs/installation.md)：動作要件、対応ブラウザー、独自ドメインでの公開
- [設定](docs/configuration.md)：サーバー設定、ストレージ、ゴミ箱、環境変数
- [アップデート](docs/updating.md)：アップデートの仕組みとスケジュールの設定
- [他のアプリからの移行](docs/migrating.md)：Zipline、XBackBone、Chibisafe のサーバーのインポート
- [AI 機能](docs/ai.md)：プリセット、モデル、スマート検索、センシティブなコンテンツ
- [Fileship for Windows](docs/desktop.md)
- [スマートフォンで使う Fileship](docs/mobile.md)：アプリのインストールと Android からの共有
- [Webhook](docs/webhooks.md)：イベント、形式、配信、署名の検証
- [テレメトリー](docs/telemetry.md)：収集されるデータとオフにする方法

## 🧰 使用技術

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

## ❤️ コントリビューション

コントリビューションを歓迎します。まず[コントリビューションガイドライン](CONTRIBUTING.md)をお読みください。コードベースのアーキテクチャと規約は [AGENTS.md](AGENTS.md) にまとめています。

## ⚖️ ライセンス

[MIT](LICENSE)
