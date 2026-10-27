<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    Öz ShareX serverin və fayl buludun: yerli Windows tətbiqi, Fayl Bələdçisi ilə sinxronizasiya, öz serverində işləyən süni intellekt axtarışı və brauzer genişləndirməsi.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Windows üçün endir</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">Xəta bildir</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">Funksiya təklif et</a>
    ·
    <a href="SECURITY.md">Təhlükəsizlik siyasəti</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <b>Azərbaycanca</b> · <a href="README.ja.md">日本語</a> · <a href="README.ko.md">한국어</a> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Fileship-ə atılan şəkil yüklənir və keçidi kopyalanır, sonra ağıllı axtarış yazılan təsvirə görə fotonu tapır və süni intellektin yaratdığı təsviri göstərir" width="100%">
</p>

## 📦 İmkanlar

- Fayllar, digər istifadəçilərlə paylaşa biləcəyin iç-içə qovluqlar, Markdown qeydləri, qısa keçidlər və gələnlər qutuları, həmçinin sildiyin hər şey üçün zibil qutusu
- Endirib bərpa edə biləcəyin hesab ehtiyat nüsxələri: əl ilə və ya cədvəl üzrə yaradılır
- Şifrə, bitmə tarixi, baxış limiti və Discord kimi tətbiqlər üçün zəngin keçid önizləmələri ilə hamıya açıq paylaşım
- Öz serverində işləyən [süni intellekt](docs/ai.md): 50-dən çox dildə ağıllı axtarış, təsvirlər, OCR və həssas məzmunun aşkarlanması; hamısı istəyə bağlıdır
- Yükləmələr üçün yerli disk, istənilən S3 uyğun xidmət, SFTP və ya FTP
- İcazələri və yaddaş limitləri olan çoxlu istifadəçi, giriş açarları, iki addımlı doğrulama və OpenID Connect ilə [SSO ilə giriş](docs/configuration.md#single-sign-on)
- Fasiləsiz yeniləmələr: bir kliklə və ya cədvəl üzrə
- ShareX, macOS və Linux üçün yükləmə inteqrasiyaları, [Windows tətbiqi](docs/desktop.md) və Android tətbiqlərindən paylaşım qəbul edən, [telefona quraşdırıla bilən tətbiq](docs/mobile.md)
- Fayl, şəkil və ekran görüntüsü yükləyən, keçidləri qısaldan [brauzer genişləndirməsi](docs/browser-extension.md): Chrome və Edge üçün [Chrome Web Store-da](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh), Firefox üçün isə mənbə kodundan qurulur
- Discord, Slack və öz xidmətlərin üçün [webhook-lar](docs/webhooks.md)
- İngilis, türk, alman, ispan, fransız, italyan, portuqal (Braziliya), rus, Azərbaycan, yapon, koreya və sadələşdirilmiş çin dilləri

## 🆚 Fileship və digərləri

|                                                        | Fileship                              | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| ------------------------------------------------------ | ------------------------------------- | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX və digər yükləmə alətləri                       | ✅                                    | ✅                                          | ✅                                                 | ✅                                                  |
| Ekran görüntüsü və qeydə alma üçün masaüstü tətbiq     | ✅ Windows                            | ❌                                          | ❌                                                 | ❌                                                  |
| Faylların Fayl Bələdçisində                            | ✅ Fileship Harbor                    | ❌                                          | ❌                                                 | ❌                                                  |
| Brauzer genişləndirməsi                                | ✅                                    | ❌                                          | ❌                                                 | ✅                                                  |
| Süni intellekt axtarışı, təsvirlər, OCR, həssas məzmun | ✅ öz serverində                      | ❌                                          | ❌                                                 | ❌                                                  |
| Qısa keçidlər                                          | ✅                                    | ✅                                          | ❌                                                 | ✅                                                  |
| Qeydlər və ya mətn paylaşımı                           | ✅ Markdown                           | ✅                                          | 🟡 mətn yükləmələri                                | ✅                                                  |
| Qovluqlar                                              | ✅ iç-içə, istifadəçilərlə paylaşılır | ✅ iç-içə                                   | ❌ teqlər                                          | 🟡 sadə albomlar                                    |
| Şifrə, bitmə tarixi və baxış limiti                    | ✅                                    | ✅                                          | ❌                                                 | 🟡 albom keçidlərinin müddəti bitir                 |
| Yaddaş                                                 | Yerli, S3, SFTP, FTP                  | Yerli, S3                                   | Yerli, S3, FTP, Dropbox, Google Cloud              | Yerli, S3                                           |
| İki addımlı doğrulama və giriş açarları                | ✅                                    | ✅                                          | ❌                                                 | ❌                                                  |
| SSO ilə giriş                                          | ✅ OIDC                               | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| Webhook-lar                                            | ✅ Discord, Slack, imzalı JSON        | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| Bərpa edilə bilən, cədvəl üzrə hesab ehtiyat nüsxələri | ✅                                    | 🟡 ixrac                                    | 🟡 ixrac                                           | ❌                                                  |
| İdarə panelindən yeniləmə                              | ✅ fasiləsiz                          | ❌                                          | 🟡 fasilə ilə                                      | ❌                                                  |
| Dillər                                                 | 12                                    | 12                                          | 31, bəziləri natamam                               | 1                                                   |
| Lisenziya                                              | MIT                                   | MIT                                         | AGPL-3.0                                           | MIT                                                 |

2026-cı ilin oktyabrında Zipline 4.8, XBackBone 3.8 və Chibisafe 6.7 ilə müqayisə edilib. Nəsə köhnəlib? [Issue aç](https://github.com/hanzydev/Fileship/issues/new/choose), düzəldək.

## 📸 Ekran görüntüləri

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="Kompüterdə Fileship idarə paneli" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="Telefonda Fileship faylları" width="18%">
</p>

## 🖥️ Windows üçün Fileship

Bildiriş sahəsindən ShareX üslubunda ekran görüntüsü çək, ekranı qeydə al və Fileship-ə yüklə, fayllarına isə Fileship Harbor ilə Fayl Bələdçisində bax. 64 bitlik Windows 11 24H2 və ya daha yeni versiya, həmçinin Fileship 6.0 serveri lazımdır.

**[Son versiyanı endir](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [İmkanlar və quraşdırma](docs/desktop.md)

## 🚀 Başlanğıc

[Docker](https://www.docker.com/) və [Docker Compose](https://docs.docker.com/compose/), 2 CPU nüvəsi və 2 GB RAM lazımdır (süni intellektlə 4 GB və ya daha çox, [tələblərə](docs/installation.md#requirements) bax).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

Fileship artıq `http://localhost:3000` ünvanında işləyir. `admin` / `password` ilə daxil ol.

> [!IMPORTANT]
> İlk girişdən dərhal sonra şifrəni dəyiş (**Ayarlar → Təhlükəsizlik**).

5.x versiyasından keçirsən? [6.0-a keçid](docs/upgrading.md) təlimatına əməl et. Zipline, XBackBone və ya Chibisafe istifadə edirdin? [Hesablarını, fayllarını və keçidlərini köçür](docs/migrating.md).

## 📚 Sənədlər

- [Quraşdırma](docs/installation.md): tələblər, brauzer dəstəyi və Fileship-in öz domenində işə salınması
- [Konfiqurasiya](docs/configuration.md): server ayarları, yaddaş, zibil qutusu və mühit dəyişənləri
- [Yeniləmə](docs/updating.md): yeniləmələr necə işləyir və onları necə planlaşdırmaq olar
- [Başqa tətbiqdən köçmək](docs/migrating.md): Zipline, XBackBone və ya Chibisafe serverinin idxalı
- [Süni intellekt imkanları](docs/ai.md): hazır ayarlar, modellər, ağıllı axtarış və həssas məzmun
- [Windows üçün Fileship](docs/desktop.md)
- [Telefonda Fileship](docs/mobile.md): tətbiqin quraşdırılması və Android-dən paylaşım
- [Webhook-lar](docs/webhooks.md): hadisələr, formatlar, çatdırılma və imzanın yoxlanması
- [Telemetriya](docs/telemetry.md): nə toplanır və necə söndürülür

## 🧰 İstifadə olunan texnologiyalar

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

## ❤️ Töhfə vermək

Töhfələrə açığıq. Əvvəlcə [töhfə qaydalarını](CONTRIBUTING.md) oxu. [AGENTS.md](AGENTS.md) kod bazasının arxitekturasını və qaydalarını təsvir edir.

## ⚖️ Lisenziya

[MIT](LICENSE)
