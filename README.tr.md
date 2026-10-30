<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    Kendi ShareX sunucun ve dosya bulutun: yerel bir Windows uygulaması, Dosya Gezgini eşitlemesi, kendi sunucunda çalışan yapay zekâ araması ve bir tarayıcı uzantısı.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Windows için indir</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Hata bildir</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Özellik iste</a>
    ·
    <a href="SECURITY.md">Güvenlik politikası</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <b>Türkçe</b> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Fileship'e bırakılan bir görsel yüklenir ve bağlantısı kopyalanır, ardından akıllı arama bir fotoğrafı tarifinden bulur ve yapay zekânın yazdığı açıklamayı gösterir" width="100%">
</p>

## 📦 Özellikler

- Dosyalar, başka kullanıcılarla paylaşabileceğin iç içe klasörler, Markdown notları, kısa bağlantılar ve gelen kutuları; sildiğin her şey için de bir çöp kutusu
- İndirip geri yükleyebileceğin hesap yedekleri, elle ya da zamanlanmış olarak oluşturulur
- Şifre, son kullanma tarihi, görüntülenme sınırı ve Discord gibi uygulamalar için [zengin yerleştirme önizlemeleriyle](docs/link-previews.md) herkese açık paylaşım
- Kendi sunucunda çalışan [yapay zekâ](docs/ai.md): 50'den fazla dilde akıllı arama, açıklamalar, OCR ve hassas içerik algılama; hepsi isteğe bağlı
- Yüklemeler için yerel disk, S3 uyumlu herhangi bir servis, SFTP veya FTP
- Yetkileri ve depolama sınırları olan birden çok kullanıcı, geçiş anahtarları, iki adımlı doğrulama ve OpenID Connect ile [tek oturum açma (SSO)](docs/configuration.md#single-sign-on)
- Kesintisiz güncellemeler: tek tıkla ya da zamanlanmış
- ShareX, macOS ve Linux için yükleme entegrasyonları, [Windows uygulaması](docs/desktop.md) ve Android uygulamalarından paylaşım alabilen, [telefona yüklenebilen bir uygulama](docs/mobile.md)
- Dosya, görsel ve ekran görüntüsü yükleyen, bağlantı kısaltan bir [tarayıcı uzantısı](docs/browser-extension.md): Chrome ve Edge için [Chrome Web Mağazası'nda](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh), Firefox için [Firefox Eklentileri'nde](https://addons.mozilla.org/firefox/addon/fileship/)
- Discord, Slack ve kendi servislerin için [webhook'lar](docs/webhooks.md)
- İngilizce, Türkçe, Almanca, İspanyolca, Fransızca, İtalyanca, Brezilya Portekizcesi, Rusça, Azerice, Japonca, Korece ve Basitleştirilmiş Çince

## 🆚 Fileship ve diğerleri

|                                                       | Fileship                                 | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| ----------------------------------------------------- | ---------------------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX ve diğer yükleme araçları                      | ✅                                       | ✅                                          | ✅                                                 | ✅                                                  |
| Ekran görüntüsü ve kayıt için masaüstü uygulaması     | ✅ Windows                               | ❌                                          | ❌                                                 | ❌                                                  |
| Dosyaların Dosya Gezgini'nde                          | ✅ Fileship Harbor                       | ❌                                          | ❌                                                 | ❌                                                  |
| Tarayıcı uzantısı                                     | ✅                                       | ❌                                          | ❌                                                 | ✅                                                  |
| Yapay zekâ araması, açıklamalar, OCR ve hassas içerik | ✅ kendi sunucunda                       | ❌                                          | ❌                                                 | ❌                                                  |
| Kısa bağlantılar                                      | ✅                                       | ✅                                          | ❌                                                 | ✅                                                  |
| Notlar veya metin paylaşımı                           | ✅ Markdown                              | ✅                                          | 🟡 metin yüklemeleri                               | ✅                                                  |
| Klasörler                                             | ✅ iç içe, kullanıcılarla paylaşılabilir | ✅ iç içe                                   | ❌ etiketler                                       | 🟡 düz albümler                                     |
| Şifre, son kullanma ve görüntülenme sınırı            | ✅                                       | ✅                                          | ❌                                                 | 🟡 albüm bağlantılarının süresi dolar               |
| Depolama                                              | Yerel, S3, SFTP, FTP                     | Yerel, S3                                   | Yerel, S3, FTP, Dropbox, Google Cloud              | Yerel, S3                                           |
| İki adımlı doğrulama ve geçiş anahtarları             | ✅                                       | ✅                                          | ❌                                                 | ❌                                                  |
| Tek oturum açma (SSO)                                 | ✅ OIDC                                  | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhook'lar                                           | ✅ Discord, Slack, imzalı JSON           | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| Geri yüklenebilen, zamanlanabilen hesap yedekleri     | ✅                                       | 🟡 dışa aktarma                             | 🟡 dışa aktarma                                    | ❌                                                  |
| Panelden güncelleme                                   | ✅ kesintisiz                            | ❌                                          | 🟡 kesintili                                       | ❌                                                  |
| Diller                                                | 12                                       | 12                                          | 31, bazıları eksik                                 | 1                                                   |
| Lisans                                                | MIT                                      | MIT                                         | AGPL-3.0                                           | MIT                                                 |

Zipline 4.8, XBackBone 3.8 ve Chibisafe 6.7 ile Ekim 2026'da karşılaştırıldı. Güncelliğini yitirmiş bir bilgi mi var? [Bir issue aç](https://github.com/hanzydev/Fileship/issues/new/choose), düzeltelim.

## 📸 Ekran görüntüleri

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Masaüstünde Fileship paneli" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Telefonda Fileship dosyaları" width="18%">
</p>

## 🖥️ Windows için Fileship

Bildirim alanından ShareX tarzında ekran görüntüsü al, kayıt yap ve Fileship'ine yükle; dosyalarına da Fileship Harbor ile Dosya Gezgini'nden göz at. 64 bit Windows 11 24H2 veya daha yeni bir sürüm ve Fileship 6.0 sunucusu gerekir.

**[Son sürümü indir](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [Özellikler ve kurulum](docs/desktop.md)

## 🚀 Başlarken

[Docker](https://www.docker.com/) ve [Docker Compose](https://docs.docker.com/compose/), 2 CPU çekirdeği ve 2 GB RAM gerekir (yapay zekâ ile 4 GB veya daha fazla, bkz. [gereksinimler](docs/installation.md#requirements)).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

Fileship artık `http://localhost:3000` adresinde çalışıyor. `admin` / `password` ile giriş yap.

> [!IMPORTANT]
> İlk girişten hemen sonra şifreni değiştir (**Ayarlar → Güvenlik**).

5.x'ten mi yükseltiyorsun? [6.0'a yükseltme](docs/upgrading.md) rehberini izle. Zipline, XBackBone veya Chibisafe kullanıyor muydun? [Hesaplarını, dosyalarını ve bağlantılarını taşı](docs/migrating.md).

## 📚 Belgeler

- [Kurulum](docs/installation.md): gereksinimler, tarayıcı desteği ve Fileship'i kendi alan adında yayınlama
- [Yapılandırma](docs/configuration.md): sunucu ayarları, depolama, çöp kutusu ve ortam değişkenleri
- [Güncelleme](docs/updating.md): güncellemelerin nasıl çalıştığı ve nasıl zamanlanacağı
- [Başka bir uygulamadan geçiş](docs/migrating.md): bir Zipline, XBackBone veya Chibisafe sunucusunu içe aktarma
- [Yapay zekâ özellikleri](docs/ai.md): ön ayarlar, modeller, akıllı arama ve hassas içerik
- [Windows için Fileship](docs/desktop.md)
- [Telefonda Fileship](docs/mobile.md): uygulamayı yükleme ve Android'den paylaşma
- [Webhook'lar](docs/webhooks.md): olaylar, biçimler, teslimat ve imza doğrulama
- [Telemetri](docs/telemetry.md): neler toplandığı ve nasıl kapatılacağı

## 🧰 Kullanılan teknolojiler

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

## ❤️ Katkıda bulunma

Katkılara açığız. Önce [katkı rehberini](CONTRIBUTING.md) oku. [AGENTS.md](AGENTS.md), kod tabanının mimarisini ve kurallarını anlatır.

## ⚖️ Lisans

[MIT](LICENSE)
