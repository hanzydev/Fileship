<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    Собственный сервер для ShareX и файловое облако: нативное приложение для Windows, синхронизация с Проводником, ИИ-поиск на вашем сервере и расширение для браузера.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Скачать для Windows</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Сообщить об ошибке</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Предложить функцию</a>
    ·
    <a href="SECURITY.md">Политика безопасности</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <b>Русский</b> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Изображение, перетащенное в Fileship, загружается, а ссылка на него копируется; затем умный поиск находит фото по текстовому запросу и показывает его описание от ИИ" width="100%">
</p>

## 📦 Возможности

- Файлы, вложенные папки с общим доступом для других пользователей, заметки в Markdown, короткие ссылки и входящие, а для всего удалённого — корзина
- Резервные копии аккаунта, которые можно скачать и восстановить; создаются вручную или по расписанию
- Публичный доступ с паролями, сроком действия, лимитом просмотров и [красивым предпросмотром ссылок](docs/link-previews.md) в Discord и других приложениях
- [ИИ](docs/ai.md), который работает на вашем сервере: умный поиск на более чем 50 языках, описания, OCR и поиск конфиденциальных данных — всё по желанию
- Хранение загрузок на локальном диске, в любом S3-совместимом сервисе, по SFTP или FTP
- Несколько пользователей с правами и лимитами хранилища, ключи доступа, двухфакторная аутентификация и [единый вход](docs/configuration.md#single-sign-on) через OpenID Connect
- Обновления без простоя: в один клик или по расписанию
- Интеграции для загрузки из ShareX, macOS и Linux, [приложение для Windows](docs/desktop.md) и [устанавливаемое приложение для телефона](docs/mobile.md), в которое можно делиться из приложений Android
- [Расширение для браузера](docs/browser-extension.md), которое загружает файлы, изображения и скриншоты и сокращает ссылки: для Chrome и Edge — [в Chrome Web Store](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh), для Firefox — [в каталоге дополнений Firefox](https://addons.mozilla.org/firefox/addon/fileship/)
- [Вебхуки](docs/webhooks.md) для Discord, Slack и ваших собственных сервисов
- Английский, турецкий, немецкий, испанский, французский, итальянский, бразильский португальский, русский, азербайджанский, японский, корейский и упрощённый китайский

## 🆚 Сравнение с другими

|                                                            | Fileship                            | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| ---------------------------------------------------------- | ----------------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX и другие загрузчики                                 | ✅                                  | ✅                                          | ✅                                                 | ✅                                                  |
| Приложение для скриншотов и записи экрана                  | ✅ Windows                          | ❌                                          | ❌                                                 | ❌                                                  |
| Ваши файлы в Проводнике                                    | ✅ Fileship Harbor                  | ❌                                          | ❌                                                 | ❌                                                  |
| Расширение для браузера                                    | ✅                                  | ❌                                          | ❌                                                 | ✅                                                  |
| ИИ-поиск, описания, OCR и конфиденциальные данные          | ✅ на вашем сервере                 | ❌                                          | ❌                                                 | ❌                                                  |
| Короткие ссылки                                            | ✅                                  | ✅                                          | ❌                                                 | ✅                                                  |
| Заметки или фрагменты текста                               | ✅ Markdown                         | ✅                                          | 🟡 загрузка текста                                 | ✅                                                  |
| Папки                                                      | ✅ вложенные, с общим доступом      | ✅ вложенные                                | ❌ теги                                            | 🟡 плоские альбомы                                  |
| Пароли, срок действия и лимит просмотров                   | ✅                                  | ✅                                          | ❌                                                 | 🟡 срок действия ссылок на альбомы                  |
| Хранилище                                                  | Локальное, S3, SFTP, FTP            | Локальное, S3                               | Локальное, S3, FTP, Dropbox, Google Cloud          | Локальное, S3                                       |
| Двухфакторная аутентификация и ключи доступа               | ✅                                  | ✅                                          | ❌                                                 | ❌                                                  |
| Единый вход                                                | ✅ OIDC                             | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Вебхуки                                                    | ✅ Discord, Slack, подписанный JSON | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| Резервные копии аккаунта с восстановлением и по расписанию | ✅                                  | 🟡 экспорт                                  | 🟡 экспорт                                         | ❌                                                  |
| Обновления из панели управления                            | ✅ без простоя                      | ❌                                          | 🟡 с простоем                                      | ❌                                                  |
| Языки                                                      | 12                                  | 12                                          | 31, часть переведена не полностью                  | 1                                                   |
| Лицензия                                                   | MIT                                 | MIT                                         | AGPL-3.0                                           | MIT                                                 |

Сравнение с Zipline 4.8, XBackBone 3.8 и Chibisafe 6.7 проведено в октябре 2026 года. Что-то устарело? [Создайте issue](https://github.com/hanzydev/Fileship/issues/new/choose), и мы всё исправим.

## 📸 Скриншоты

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Панель управления Fileship на компьютере" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Файлы в Fileship на телефоне" width="18%">
</p>

## 🖥️ Fileship для Windows

Делайте скриншоты, записывайте экран и загружайте всё в свой Fileship прямо из области уведомлений, как в ShareX, а с Fileship Harbor работайте со своими файлами в Проводнике. Нужна 64-разрядная Windows 11 версии 24H2 или новее и сервер Fileship 6.0.

**[Скачать последнюю версию](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [Возможности и настройка](docs/desktop.md)

## 🚀 Быстрый старт

Понадобятся [Docker](https://www.docker.com/) с [Docker Compose](https://docs.docker.com/compose/), 2 ядра CPU и 2 ГБ RAM (с ИИ — от 4 ГБ, см. [требования](docs/installation.md#requirements)).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

Теперь Fileship доступен по адресу `http://localhost:3000`. Войдите как `admin` / `password`.

> [!IMPORTANT]
> Смените пароль сразу после первого входа (**Настройки → Безопасность**).

Обновляетесь с 5.x? Следуйте инструкции [Переход на 6.0](docs/upgrading.md). Переходите с Zipline, XBackBone или Chibisafe? [Перенесите аккаунты, файлы и ссылки](docs/migrating.md).

## 📚 Документация

- [Установка](docs/installation.md): требования, поддержка браузеров и запуск Fileship на собственном домене
- [Настройка](docs/configuration.md): настройки сервера, хранилище, корзина и переменные окружения
- [Обновление](docs/updating.md): как работают обновления и как их запланировать
- [Переход с другого приложения](docs/migrating.md): импорт сервера Zipline, XBackBone или Chibisafe
- [Возможности ИИ](docs/ai.md): профили, модели, умный поиск и конфиденциальные данные
- [Fileship для Windows](docs/desktop.md)
- [Fileship на телефоне](docs/mobile.md): установка приложения и отправка файлов из Android
- [Вебхуки](docs/webhooks.md): события, форматы, доставка и проверка подписи
- [Телеметрия](docs/telemetry.md): что собирается и как это отключить

## 🧰 Технологии

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

## ❤️ Участие в разработке

Мы рады любому вкладу. Сначала прочитайте [правила участия](CONTRIBUTING.md). В [AGENTS.md](AGENTS.md) описаны архитектура и соглашения кодовой базы.

## ⚖️ Лицензия

[MIT](LICENSE)
