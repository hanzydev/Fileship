# Store listing

Everything to fill in when the extension is submitted to the Chrome Web Store and addons.mozilla.org (AMO). Paste the text blocks as they are. Version 1.0.0 is [on the Chrome Web Store](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh) (item id `lbnlfnmlaipoflalhikladilalkoagnh`); AMO and Edge Add-ons are not done yet. Replace `<SERVER_URL>` and `<UPLOAD_TOKEN>` with the reviewer account's values in the store forms only; never commit them.

## Packages

Run `pnpm --filter extension package`. It writes to `apps/extension/.output`:

| File                             | Where it goes                             |
| -------------------------------- | ----------------------------------------- |
| `fileship-<version>-chrome.zip`  | Chrome Web Store (and Edge Add-ons later) |
| `fileship-<version>-firefox.zip` | AMO, the add-on itself                    |
| `fileship-<version>-sources.zip` | AMO, the source code step                 |

Before an AMO submission, rebuild the sources zip in an empty folder (steps in `docs/browser-extension.md`) and check that `firefox-mv3` matches the Firefox zip.

## Shared details

| Field          | Value                                                                            |
| -------------- | -------------------------------------------------------------------------------- |
| Name           | Fileship                                                                         |
| Homepage       | https://github.com/hanzydev/Fileship                                             |
| Support        | https://github.com/hanzydev/Fileship/issues                                      |
| Privacy policy | https://github.com/hanzydev/Fileship/blob/main/docs/browser-extension-privacy.md |
| License        | MIT                                                                              |
| Images         | `apps/extension/store/images`                                                    |

The reviewer account: a user on a public Fileship 6.0+ server with only the **Upload files** and **Shorten links** permissions and a small usable space (for example 100 MB). Its upload token is on **Settings → Integrations**. Keep it while store reviews may happen; regenerating the token breaks the review notes.

## Chrome Web Store

### Store listing tab

- **Description:** the English text below for the default language. Add the other languages with the language picker; Chrome has no Azerbaijani, so skip that one. The short description comes from the package (`_locales`), so there is nothing to type for it.
- **Category:** Productivity → Tools
- **Store icon:** `images/icon-128.png`
- **Screenshots:** the `images/screenshot-*.png` files (1280×800), in order
- **Small promo tile:** `images/promo-small-440x280.png`; **Marquee:** `images/promo-marquee-1400x560.png`
- **Official URL:** none needed. **Homepage URL** and **Support URL:** from the table above

### Privacy tab

Single purpose:

```text
Upload files, images and screenshots to the user's own Fileship server and shorten links with it.
```

Permission justifications:

| Permission      | Justification                                                                                                                                                                                                                              |
| --------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| activeTab       | Reads the address of the current tab to shorten it and captures the visible tab, only after the user clicks the toolbar button or a context menu item.                                                                                     |
| clipboardWrite  | Copies the link of a finished upload or a new short link to the clipboard.                                                                                                                                                                 |
| contextMenus    | Adds "Upload image to Fileship", "Shorten link with Fileship" and "Shorten page link with Fileship" to the right-click menu.                                                                                                               |
| notifications   | Shows the link after an upload or a shortened link, or explains what went wrong.                                                                                                                                                           |
| scripting       | Injects the area picker into the current tab when the user chooses "Capture an area", and reads a same-site image inside the page when the user uploads it from the context menu.                                                          |
| storage         | Keeps the server address, the upload token, the upload options and the list of recent uploads on this computer.                                                                                                                            |
| offscreen       | Copies the link to the clipboard from the background, because the service worker cannot reach the clipboard (reason: CLIPBOARD).                                                                                                           |
| Host permission | Fileship is self-hosted, so the server can be on any domain. Nothing is granted at install: the extension asks at runtime only for the origin of the server the user enters, and for the origin of an image only when the user uploads it. |

Remote code: **No, I am not using remote code.**

Data usage, check these:

- **Authentication information:** the upload token, sent only to the user's own server to authorize uploads.
- **Web history:** the address of a page or link, only when the user chooses to shorten it.
- **Website content:** images and screenshots, only when the user uploads them.

Check all three certifications, and use the privacy policy URL from the table above.

### Test instructions tab

Leave **Username** and **Password** empty: the extension never logs in, and the password would let reviewers regenerate the token. **Additional instructions** takes at most 500 characters; this text is 489 with a 44-character token:

```text
No login needed, only this server and upload token:
Server address: <SERVER_URL>
Upload token: <UPLOAD_TOKEN>
1. Toolbar button > Open settings. Enter both, click Test connection, then Save, and allow access to the server.
2. Toolbar button: Upload files, Shorten this page, Capture visible area, Capture an area. Or right-click an image (Upload image to Fileship) or a link (Shorten link with Fileship).
The link is copied and shown in a notification.
```

### Distribution tab

Free, **Public**, all regions.

## addons.mozilla.org

1. **Submit a New Add-on → On this site.**
2. Upload `fileship-<version>-firefox.zip`. Platform: **Firefox** only (the context menus do not exist on Android).
3. **Do you need to submit source code?** Yes, upload `fileship-<version>-sources.zip`.
4. Listing: name **Fileship**, URL `fileship` if it is free, the **Summary** from the texts by language and the **Description** from the AMO descriptions in Markdown at the end (add the other languages from the language picker on the edit page), categories **Photos, Music & Videos** and **Social & Communication**, support website from the table above, license **MIT**.
5. Privacy policy: tick **This add-on has a privacy policy** and paste the text of `docs/browser-extension-privacy.md`.
6. Screenshots (optional on AMO): the same `images/screenshot-*.png` files.
7. Notes to Reviewer:

```text
Fileship is a self-hosted file server; the extension needs one to do anything.

Source code: the add-on is built with WXT and Vite from a pnpm monorepo. The sources zip contains the extension (apps/extension), the shared UI layer it imports (packages/ui), the ESLint config, the root .gitignore and the lockfile. In the unzipped folder, on any OS with Node.js 22 or newer (tested with 24) and pnpm 11.9.0:

corepack enable
pnpm install --frozen-lockfile
pnpm --filter extension build

apps/extension/.output/firefox-mv3 then holds exactly the files of the submitted package. The same steps are in docs/browser-extension.md.

Testing:
Server address: <SERVER_URL>
Upload token: <UPLOAD_TOKEN>

1. Toolbar button, "Open settings". Enter the server address and the upload token, choose "Test connection", then "Save", and allow access to the server.
2. Toolbar button: "Upload files" (opens an upload tab in Firefox), "Shorten this page", "Capture visible area", "Capture an area".
3. Right-click an image: "Upload image to Fileship"; a link: "Shorten link with Fileship".

Data: the add-on sends data only to the server the user configures and only when the user uses one of its actions (declared as websiteContent and browsingActivity). No analytics, no remote code.

The "Unsafe assignment to innerHTML" warning comes from the Vue runtime (runtime-dom: static content and the innerHTML prop). The add-on's own code never sets innerHTML and uses no v-html.
```

Version notes:

```text
First release: upload files, images and screenshots to your own Fileship server and shorten links, from the toolbar button or the right-click menu.
```

## Texts by language

The summary is the package's own description (Chrome reads it from the package; AMO asks for it as **Summary**). The description goes into both stores.

### English (en, default)

Summary:

```text
Upload files, screenshots and images to your Fileship and shorten links.
```

Description:

```text
Upload files, images and screenshots to your own Fileship server and shorten links, right from your browser.

Fileship is an open-source, self-hosted platform for files, notes and short links. This extension connects to your Fileship server with your upload token.

What you can do
• Upload files from the toolbar button, or drop them on the upload page
• Capture the visible part of a tab, or drag over the page to capture an area
• Right-click an image to upload it, or a link to shorten it
• Shorten the address of the current tab
• The link is copied to the clipboard and shown in a notification
• Follow your uploads in the toolbar button, with progress, copy buttons and clear error messages
• Large files upload in parallel chunks that follow your server's limits
• Choose the default file name style and expiry

Privacy
The extension talks only to the server you enter. It has no analytics or tracking and sends something only when you use one of its actions. It asks for no site access at install, only for access to your server when you save the connection.

What you need
• A Fileship 6.0 server or newer: https://github.com/hanzydev/Fileship
• An upload token from Settings → Integrations in your Fileship dashboard

Available in English, Turkish, German, Spanish, French, Italian, Brazilian Portuguese, Russian, Azerbaijani, Japanese, Korean and Simplified Chinese. Open source under the MIT license.
```

### Türkçe (tr)

Summary:

```text
Dosyaları, ekran görüntülerini ve resimleri Fileship'e yükle, bağlantıları kısalt.
```

Description:

```text
Dosyaları, resimleri ve ekran görüntülerini kendi Fileship sunucuna yükle, bağlantıları kısalt; hepsi doğrudan tarayıcından.

Fileship; dosyalar, notlar ve kısa bağlantılar için açık kaynaklı, kendi sunucunda çalışan bir platform. Bu eklenti, yükleme anahtarınla Fileship sunucuna bağlanır.

Neler yapabilirsin
• Araç çubuğu düğmesinden dosya yükle ya da dosyaları yükleme sayfasına bırak
• Sekmenin görünen alanını yakala ya da sayfada sürükleyerek bir bölge seç
• Bir resme sağ tıklayıp yükle, bir bağlantıya sağ tıklayıp kısalt
• Açık sekmenin adresini kısalt
• Bağlantı panoya kopyalanır ve bildirimde gösterilir
• Yüklemelerini araç çubuğu düğmesinden takip et: ilerleme, kopyalama düğmeleri ve anlaşılır hata mesajları
• Büyük dosyalar, sunucunun sınırlarına uyan parçalar hâlinde paralel yüklenir
• Varsayılan dosya adı biçimini ve son kullanma süresini seç

Gizlilik
Eklenti yalnızca girdiğin sunucuyla konuşur. Analitik ya da takip içermez ve yalnızca eklentinin bir eylemini kullandığında bir şey gönderir. Kurulurken hiçbir siteye erişim istemez; sunucuna erişimi yalnızca bağlantıyı kaydettiğinde ister.

Gerekenler
• Fileship 6.0 veya daha yeni bir sunucu: https://github.com/hanzydev/Fileship
• Fileship panelinde Ayarlar → Entegrasyonlar sayfasından bir yükleme anahtarı

İngilizce, Türkçe, Almanca, İspanyolca, Fransızca, İtalyanca, Brezilya Portekizcesi, Rusça, Azerbaycanca, Japonca, Korece ve Basitleştirilmiş Çince olarak kullanılabilir. MIT lisanslı, açık kaynak.
```

### Deutsch (de)

Summary:

```text
Lade Dateien, Screenshots und Bilder in dein Fileship hoch und kürze Links.
```

Description:

```text
Lade Dateien, Bilder und Screenshots auf deinen eigenen Fileship-Server hoch und kürze Links, direkt im Browser.

Fileship ist eine selbst gehostete Open-Source-Plattform für Dateien, Notizen und Kurzlinks. Diese Erweiterung verbindet sich mit deinem Upload-Token mit deinem Fileship-Server.

Was du tun kannst
• Dateien über die Schaltfläche in der Symbolleiste hochladen oder auf der Upload-Seite ablegen
• Den sichtbaren Bereich eines Tabs aufnehmen oder auf der Seite einen Bereich aufziehen
• Ein Bild per Rechtsklick hochladen oder einen Link kürzen
• Die Adresse des aktuellen Tabs kürzen
• Der Link wird in die Zwischenablage kopiert und in einer Benachrichtigung angezeigt
• Uploads über die Schaltfläche in der Symbolleiste verfolgen, mit Fortschritt, Kopieren-Schaltflächen und verständlichen Fehlermeldungen
• Große Dateien werden in parallelen Teilen hochgeladen, passend zu den Limits deines Servers
• Standard für Dateinamen und Ablauf festlegen

Datenschutz
Die Erweiterung kommuniziert nur mit dem Server, den du einträgst. Sie enthält keine Analyse und kein Tracking und sendet nur etwas, wenn du eine ihrer Aktionen nutzt. Bei der Installation fragt sie nach keinem Zugriff auf Websites, sondern erst beim Speichern der Verbindung nach dem Zugriff auf deinen Server.

Was du brauchst
• Einen Fileship-Server ab Version 6.0: https://github.com/hanzydev/Fileship
• Ein Upload-Token aus Einstellungen → Integrationen in deinem Fileship-Dashboard

Verfügbar auf Englisch, Türkisch, Deutsch, Spanisch, Französisch, Italienisch, brasilianischem Portugiesisch, Russisch, Aserbaidschanisch, Japanisch, Koreanisch und vereinfachtem Chinesisch. Open Source unter der MIT-Lizenz.
```

### Español (es)

Summary:

```text
Sube archivos, capturas de pantalla e imágenes a tu Fileship y acorta enlaces.
```

Description:

```text
Sube archivos, imágenes y capturas de pantalla a tu propio servidor de Fileship y acorta enlaces, directamente desde el navegador.

Fileship es una plataforma de código abierto y autoalojada para archivos, notas y enlaces cortos. Esta extensión se conecta a tu servidor de Fileship con tu token de subida.

Qué puedes hacer
• Subir archivos desde el botón de la barra de herramientas o soltarlos en la página de subida
• Capturar la parte visible de una pestaña o arrastrar sobre la página para capturar un área
• Hacer clic derecho en una imagen para subirla o en un enlace para acortarlo
• Acortar la dirección de la pestaña actual
• El enlace se copia al portapapeles y se muestra en una notificación
• Seguir tus subidas desde el botón de la barra de herramientas, con el progreso, botones para copiar y mensajes de error claros
• Los archivos grandes se suben en fragmentos paralelos que respetan los límites de tu servidor
• Elegir el estilo de nombre de archivo y la caducidad predeterminados

Privacidad
La extensión solo se comunica con el servidor que indiques. No incluye analíticas ni seguimiento y solo envía algo cuando usas una de sus acciones. No pide acceso a ningún sitio al instalarse, solo acceso a tu servidor cuando guardas la conexión.

Qué necesitas
• Un servidor de Fileship 6.0 o posterior: https://github.com/hanzydev/Fileship
• Un token de subida de Configuración → Integraciones en tu panel de Fileship

Disponible en inglés, turco, alemán, español, francés, italiano, portugués de Brasil, ruso, azerí, japonés, coreano y chino simplificado. Código abierto con licencia MIT.
```

### Français (fr)

Summary:

```text
Importez des fichiers, des captures d'écran et des images dans votre Fileship et raccourcissez des liens.
```

Description:

```text
Importez des fichiers, des images et des captures d'écran sur votre propre serveur Fileship et raccourcissez des liens, directement depuis votre navigateur.

Fileship est une plateforme open source et auto-hébergée pour les fichiers, les notes et les liens courts. Cette extension se connecte à votre serveur Fileship avec votre jeton d'importation.

Ce que vous pouvez faire
• Importer des fichiers depuis le bouton de la barre d'outils, ou les déposer sur la page d'importation
• Capturer la zone visible d'un onglet, ou faire glisser sur la page pour capturer une zone
• Faire un clic droit sur une image pour l'importer, ou sur un lien pour le raccourcir
• Raccourcir l'adresse de l'onglet actif
• Le lien est copié dans le presse-papiers et affiché dans une notification
• Suivre vos importations depuis le bouton de la barre d'outils, avec la progression, des boutons de copie et des messages d'erreur clairs
• Les gros fichiers sont importés en plusieurs parties en parallèle, selon les limites de votre serveur
• Choisir le style de nom de fichier et l'expiration par défaut

Confidentialité
L'extension ne communique qu'avec le serveur que vous indiquez. Elle ne contient ni statistiques ni suivi et n'envoie rien tant que vous n'utilisez pas l'une de ses actions. À l'installation, elle ne demande l'accès à aucun site ; l'accès à votre serveur n'est demandé qu'à l'enregistrement de la connexion.

Prérequis
• Un serveur Fileship 6.0 ou plus récent : https://github.com/hanzydev/Fileship
• Un jeton d'importation, depuis Paramètres → Intégrations dans votre tableau de bord Fileship

Disponible en anglais, turc, allemand, espagnol, français, italien, portugais du Brésil, russe, azerbaïdjanais, japonais, coréen et chinois simplifié. Open source sous licence MIT.
```

### Italiano (it)

Summary:

```text
Carica file, screenshot e immagini sul tuo Fileship e abbrevia i link.
```

Description:

```text
Carica file, immagini e screenshot sul tuo server Fileship e abbrevia i link, direttamente dal browser.

Fileship è una piattaforma open source e self-hosted per file, note e link brevi. Questa estensione si collega al tuo server Fileship con il tuo token di caricamento.

Cosa puoi fare
• Caricare file dal pulsante nella barra degli strumenti o trascinarli nella pagina di caricamento
• Acquisire l'area visibile di una scheda o trascinare sulla pagina per acquisire un'area
• Fare clic destro su un'immagine per caricarla o su un link per abbreviarlo
• Abbreviare l'indirizzo della scheda attiva
• Il link viene copiato negli appunti e mostrato in una notifica
• Seguire i caricamenti dal pulsante nella barra degli strumenti, con avanzamento, pulsanti per copiare e messaggi di errore chiari
• I file grandi vengono caricati in parti parallele, secondo i limiti del tuo server
• Scegliere lo stile predefinito del nome file e la scadenza

Privacy
L'estensione comunica solo con il server che inserisci. Non contiene statistiche né tracciamento e invia qualcosa solo quando usi una delle sue azioni. All'installazione non chiede l'accesso ad alcun sito: l'accesso al tuo server viene chiesto solo quando salvi la connessione.

Requisiti
• Un server Fileship 6.0 o successivo: https://github.com/hanzydev/Fileship
• Un token di caricamento da Impostazioni → Integrazioni nella dashboard di Fileship

Disponibile in inglese, turco, tedesco, spagnolo, francese, italiano, portoghese brasiliano, russo, azero, giapponese, coreano e cinese semplificato. Open source con licenza MIT.
```

### Português (Brasil) (pt_BR)

Summary:

```text
Envie arquivos, capturas de tela e imagens para o seu Fileship e encurte links.
```

Description:

```text
Envie arquivos, imagens e capturas de tela para o seu próprio servidor Fileship e encurte links, direto do navegador.

O Fileship é uma plataforma de código aberto e auto-hospedada para arquivos, notas e links curtos. Esta extensão se conecta ao seu servidor Fileship com o seu token de envio.

O que você pode fazer
• Enviar arquivos pelo botão da barra de ferramentas ou soltá-los na página de envio
• Capturar a parte visível de uma aba ou arrastar sobre a página para capturar uma área
• Clicar com o botão direito em uma imagem para enviá-la ou em um link para encurtá-lo
• Encurtar o endereço da aba atual
• O link é copiado para a área de transferência e mostrado em uma notificação
• Acompanhar os envios pelo botão da barra de ferramentas, com o progresso, botões de copiar e mensagens de erro claras
• Arquivos grandes são enviados em partes paralelas que respeitam os limites do seu servidor
• Escolher o estilo padrão do nome de arquivo e a expiração

Privacidade
A extensão se comunica apenas com o servidor que você informar. Ela não tem análises nem rastreamento e só envia algo quando você usa uma das suas ações. Na instalação, não pede acesso a nenhum site, apenas ao seu servidor quando você salva a conexão.

O que você precisa
• Um servidor Fileship 6.0 ou mais recente: https://github.com/hanzydev/Fileship
• Um token de envio em Configurações → Integrações no painel do Fileship

Disponível em inglês, turco, alemão, espanhol, francês, italiano, português do Brasil, russo, azerbaijano, japonês, coreano e chinês simplificado. Código aberto sob a licença MIT.
```

### Русский (ru)

Summary:

```text
Загружайте файлы, скриншоты и изображения в свой Fileship и сокращайте ссылки.
```

Description:

```text
Загружайте файлы, изображения и скриншоты на свой сервер Fileship и сокращайте ссылки прямо из браузера.

Fileship — платформа с открытым исходным кодом для файлов, заметок и коротких ссылок, которую вы размещаете на своём сервере. Расширение подключается к вашему серверу Fileship по токену загрузки.

Что можно делать
• Загружать файлы кнопкой на панели инструментов или перетаскивать их на страницу загрузки
• Делать скриншот видимой части вкладки или выделять на странице область для захвата
• Загружать изображение или сокращать ссылку из контекстного меню
• Сокращать адрес текущей вкладки
• Ссылка копируется в буфер обмена и показывается в уведомлении
• Следить за загрузками с кнопки на панели инструментов: прогресс, кнопки копирования и понятные сообщения об ошибках
• Большие файлы загружаются параллельными частями с учётом ограничений вашего сервера
• Выбирать стиль имени файла и срок действия по умолчанию

Конфиденциальность
Расширение обращается только к указанному вами серверу. В нём нет аналитики и отслеживания, и оно отправляет данные, только когда вы используете одно из его действий. При установке оно не запрашивает доступ к сайтам — только доступ к вашему серверу, когда вы сохраняете подключение.

Что нужно
• Сервер Fileship версии 6.0 или новее: https://github.com/hanzydev/Fileship
• Токен загрузки из раздела «Настройки → Интеграции» в панели управления Fileship

Доступно на английском, турецком, немецком, испанском, французском, итальянском, бразильском португальском, русском, азербайджанском, японском, корейском и упрощённом китайском. Открытый исходный код под лицензией MIT.
```

### Azərbaycanca (az, AMO only)

Summary:

```text
Faylları, ekran görüntülərini və şəkilləri Fileship-ə yüklə, keçidləri qısalt.
```

Description:

```text
Faylları, şəkilləri və ekran görüntülərini öz Fileship serverinə yüklə və keçidləri qısalt, birbaşa brauzerindən.

Fileship fayllar, qeydlər və qısa keçidlər üçün açıq mənbəli, öz serverində işləyən platformadır. Bu genişləndirmə yükləmə açarınla Fileship serverinə qoşulur.

Nələr edə bilərsən
• Alətlər panelindəki düymədən fayl yüklə və ya faylları yükləmə səhifəsinə burax
• Tabın görünən sahəsini çək və ya səhifədə sürükləyərək bir sahə seç
• Şəklə sağ klikləyib yüklə, keçidə sağ klikləyib qısalt
• Açıq tabın ünvanını qısalt
• Keçid mübadilə buferinə kopyalanır və bildirişdə göstərilir
• Yükləmələrini alətlər panelindəki düymədən izlə: irəliləyiş, kopyalama düymələri və aydın xəta mesajları
• Böyük fayllar serverinin limitlərinə uyğun hissələrlə paralel yüklənir
• Standart fayl adı üslubunu və bitmə müddətini seç

Məxfilik
Genişləndirmə yalnız daxil etdiyin serverlə əlaqə saxlayır. Analitika və izləmə yoxdur və o, yalnız sən onun əməliyyatlarından birini istifadə etdikdə nəsə göndərir. Quraşdırılarkən heç bir sayta giriş istəmir, serverinə girişi isə yalnız bağlantını yadda saxladıqda istəyir.

Nə lazımdır
• Fileship 6.0 və ya daha yeni server: https://github.com/hanzydev/Fileship
• Fileship idarə panelində Ayarlar → İnteqrasiyalar səhifəsindən yükləmə açarı

İngilis, türk, alman, ispan, fransız, italyan, Braziliya portuqal, rus, Azərbaycan, yapon, koreya və sadələşdirilmiş Çin dillərində mövcuddur. MIT lisenziyası ilə açıq mənbəli.
```

### 日本語 (ja)

Summary:

```text
ファイル、スクリーンショット、画像を Fileship にアップロードし、リンクを短縮します。
```

Description:

```text
ファイル、画像、スクリーンショットを自分の Fileship サーバーにアップロードし、リンクを短縮できます。すべてブラウザーから直接操作できます。

Fileship は、ファイル、ノート、短縮リンクのためのオープンソースのセルフホスト型プラットフォームです。この拡張機能は、アップロードトークンを使って Fileship サーバーに接続します。

できること
• ツールバーのボタンからファイルをアップロード、またはアップロードページにドロップ
• タブの表示範囲をキャプチャ、またはページ上をドラッグして範囲をキャプチャ
• 画像を右クリックしてアップロード、リンクを右クリックして短縮
• 現在のタブのアドレスを短縮
• リンクはクリップボードにコピーされ、通知に表示されます
• ツールバーのボタンでアップロードを確認：進行状況、コピーボタン、わかりやすいエラーメッセージ
• 大きなファイルは、サーバーの制限に合わせて分割し、並列でアップロード
• 既定のファイル名の形式と有効期限を選択

プライバシー
拡張機能が通信するのは、入力したサーバーだけです。分析やトラッキングはなく、拡張機能の操作を使ったときにだけ送信します。インストール時にはどのサイトへのアクセスも求めず、サーバーへのアクセスは接続を保存したときにだけ求めます。

必要なもの
• Fileship 6.0 以降のサーバー：https://github.com/hanzydev/Fileship
• Fileship のダッシュボードの「設定」→「連携」で取得できるアップロードトークン

英語、トルコ語、ドイツ語、スペイン語、フランス語、イタリア語、ポルトガル語（ブラジル）、ロシア語、アゼルバイジャン語、日本語、韓国語、簡体字中国語に対応しています。MIT ライセンスのオープンソースです。
```

### 한국어 (ko)

Summary:

```text
파일, 스크린샷, 이미지를 Fileship에 업로드하고 링크를 단축해요.
```

Description:

```text
파일, 이미지, 스크린샷을 내 Fileship 서버에 업로드하고 링크를 단축해요. 모두 브라우저에서 바로 할 수 있어요.

Fileship은 파일, 노트, 단축 링크를 위한 오픈 소스 셀프 호스팅 플랫폼이에요. 이 확장 프로그램은 업로드 토큰으로 Fileship 서버에 연결해요.

할 수 있는 일
• 툴바 버튼으로 파일을 업로드하거나 업로드 페이지에 끌어다 놓기
• 탭의 보이는 영역을 캡처하거나 페이지에서 드래그해 영역 캡처
• 이미지를 마우스 오른쪽 버튼으로 클릭해 업로드하고, 링크는 단축
• 현재 탭의 주소 단축
• 링크는 클립보드에 복사되고 알림으로 표시돼요
• 툴바 버튼에서 업로드 진행 상황, 복사 버튼, 알기 쉬운 오류 메시지 확인
• 큰 파일은 서버 제한에 맞춰 나눠서 동시에 업로드
• 기본 파일 이름 형식과 만료 기간 선택

개인정보 보호
확장 프로그램은 입력한 서버와만 통신해요. 분석이나 추적 기능이 없고, 확장 프로그램의 기능을 사용할 때만 데이터를 보내요. 설치할 때는 어떤 사이트에 대한 접근 권한도 요청하지 않고, 연결을 저장할 때만 서버 접근 권한을 요청해요.

필요한 것
• Fileship 6.0 이상 서버: https://github.com/hanzydev/Fileship
• Fileship 대시보드의 설정 → 연동에서 받은 업로드 토큰

영어, 터키어, 독일어, 스페인어, 프랑스어, 이탈리아어, 포르투갈어(브라질), 러시아어, 아제르바이잔어, 일본어, 한국어, 중국어(간체)를 지원해요. MIT 라이선스의 오픈 소스예요.
```

### 简体中文 (zh_CN)

Summary:

```text
将文件、截图和图片上传到你的 Fileship，并缩短链接。
```

Description:

```text
将文件、图片和截图上传到你自己的 Fileship 服务器，并缩短链接，全部在浏览器中完成。

Fileship 是一个开源、自托管的平台，用于管理文件、笔记和短链接。此扩展使用你的上传令牌连接到你的 Fileship 服务器。

你可以
• 通过工具栏按钮上传文件，或将文件拖放到上传页面
• 截取标签页的可见区域，或在页面上拖动以截取某个区域
• 右键点击图片即可上传，右键点击链接即可缩短
• 缩短当前标签页的地址
• 链接会复制到剪贴板，并显示在通知中
• 在工具栏按钮中查看上传进度、复制按钮和清晰的错误信息
• 大文件会按照服务器的限制分块并行上传
• 选择默认的文件名样式和过期时间

隐私
扩展只与你填写的服务器通信。它没有任何统计或跟踪功能，只有在你使用它的某项功能时才会发送数据。安装时不请求访问任何网站，只有在你保存连接时才请求访问你的服务器。

你需要
• Fileship 6.0 或更高版本的服务器：https://github.com/hanzydev/Fileship
• 在 Fileship 控制台的“设置 → 集成”中获取的上传令牌

支持英语、土耳其语、德语、西班牙语、法语、意大利语、巴西葡萄牙语、俄语、阿塞拜疆语、日语、韩语和简体中文。基于 MIT 许可证开源。
```

## AMO descriptions (Markdown)

AMO renders a limited Markdown in the description, so it gets these versions with real lists and bold section titles; Chrome keeps the plain text above. Paste each into the **Description** of its language.

### English (en, default)

```text
Upload files, images and screenshots to your own Fileship server and shorten links, right from your browser.

Fileship is an open-source, self-hosted platform for files, notes and short links. This extension connects to your Fileship server with your upload token.

**What you can do**

- Upload files from the toolbar button, or drop them on the upload page
- Capture the visible part of a tab, or drag over the page to capture an area
- Right-click an image to upload it, or a link to shorten it
- Shorten the address of the current tab
- The link is copied to the clipboard and shown in a notification
- Follow your uploads in the toolbar button, with progress, copy buttons and clear error messages
- Large files upload in parallel chunks that follow your server's limits
- Choose the default file name style and expiry

**Privacy**

The extension talks only to the server you enter. It has no analytics or tracking and sends something only when you use one of its actions. It asks for no site access at install, only for access to your server when you save the connection.

**What you need**

- A Fileship 6.0 server or newer: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- An upload token from Settings → Integrations in your Fileship dashboard

Available in English, Turkish, German, Spanish, French, Italian, Brazilian Portuguese, Russian, Azerbaijani, Japanese, Korean and Simplified Chinese. Open source under the MIT license.
```

### Türkçe (tr)

```text
Dosyaları, resimleri ve ekran görüntülerini kendi Fileship sunucuna yükle, bağlantıları kısalt; hepsi doğrudan tarayıcından.

Fileship; dosyalar, notlar ve kısa bağlantılar için açık kaynaklı, kendi sunucunda çalışan bir platform. Bu eklenti, yükleme anahtarınla Fileship sunucuna bağlanır.

**Neler yapabilirsin**

- Araç çubuğu düğmesinden dosya yükle ya da dosyaları yükleme sayfasına bırak
- Sekmenin görünen alanını yakala ya da sayfada sürükleyerek bir bölge seç
- Bir resme sağ tıklayıp yükle, bir bağlantıya sağ tıklayıp kısalt
- Açık sekmenin adresini kısalt
- Bağlantı panoya kopyalanır ve bildirimde gösterilir
- Yüklemelerini araç çubuğu düğmesinden takip et: ilerleme, kopyalama düğmeleri ve anlaşılır hata mesajları
- Büyük dosyalar, sunucunun sınırlarına uyan parçalar hâlinde paralel yüklenir
- Varsayılan dosya adı biçimini ve son kullanma süresini seç

**Gizlilik**

Eklenti yalnızca girdiğin sunucuyla konuşur. Analitik ya da takip içermez ve yalnızca eklentinin bir eylemini kullandığında bir şey gönderir. Kurulurken hiçbir siteye erişim istemez; sunucuna erişimi yalnızca bağlantıyı kaydettiğinde ister.

**Gerekenler**

- Fileship 6.0 veya daha yeni bir sunucu: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Fileship panelinde Ayarlar → Entegrasyonlar sayfasından bir yükleme anahtarı

İngilizce, Türkçe, Almanca, İspanyolca, Fransızca, İtalyanca, Brezilya Portekizcesi, Rusça, Azerbaycanca, Japonca, Korece ve Basitleştirilmiş Çince olarak kullanılabilir. MIT lisanslı, açık kaynak.
```

### Deutsch (de)

```text
Lade Dateien, Bilder und Screenshots auf deinen eigenen Fileship-Server hoch und kürze Links, direkt im Browser.

Fileship ist eine selbst gehostete Open-Source-Plattform für Dateien, Notizen und Kurzlinks. Diese Erweiterung verbindet sich mit deinem Upload-Token mit deinem Fileship-Server.

**Was du tun kannst**

- Dateien über die Schaltfläche in der Symbolleiste hochladen oder auf der Upload-Seite ablegen
- Den sichtbaren Bereich eines Tabs aufnehmen oder auf der Seite einen Bereich aufziehen
- Ein Bild per Rechtsklick hochladen oder einen Link kürzen
- Die Adresse des aktuellen Tabs kürzen
- Der Link wird in die Zwischenablage kopiert und in einer Benachrichtigung angezeigt
- Uploads über die Schaltfläche in der Symbolleiste verfolgen, mit Fortschritt, Kopieren-Schaltflächen und verständlichen Fehlermeldungen
- Große Dateien werden in parallelen Teilen hochgeladen, passend zu den Limits deines Servers
- Standard für Dateinamen und Ablauf festlegen

**Datenschutz**

Die Erweiterung kommuniziert nur mit dem Server, den du einträgst. Sie enthält keine Analyse und kein Tracking und sendet nur etwas, wenn du eine ihrer Aktionen nutzt. Bei der Installation fragt sie nach keinem Zugriff auf Websites, sondern erst beim Speichern der Verbindung nach dem Zugriff auf deinen Server.

**Was du brauchst**

- Einen Fileship-Server ab Version 6.0: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Ein Upload-Token aus Einstellungen → Integrationen in deinem Fileship-Dashboard

Verfügbar auf Englisch, Türkisch, Deutsch, Spanisch, Französisch, Italienisch, brasilianischem Portugiesisch, Russisch, Aserbaidschanisch, Japanisch, Koreanisch und vereinfachtem Chinesisch. Open Source unter der MIT-Lizenz.
```

### Español (es)

```text
Sube archivos, imágenes y capturas de pantalla a tu propio servidor de Fileship y acorta enlaces, directamente desde el navegador.

Fileship es una plataforma de código abierto y autoalojada para archivos, notas y enlaces cortos. Esta extensión se conecta a tu servidor de Fileship con tu token de subida.

**Qué puedes hacer**

- Subir archivos desde el botón de la barra de herramientas o soltarlos en la página de subida
- Capturar la parte visible de una pestaña o arrastrar sobre la página para capturar un área
- Hacer clic derecho en una imagen para subirla o en un enlace para acortarlo
- Acortar la dirección de la pestaña actual
- El enlace se copia al portapapeles y se muestra en una notificación
- Seguir tus subidas desde el botón de la barra de herramientas, con el progreso, botones para copiar y mensajes de error claros
- Los archivos grandes se suben en fragmentos paralelos que respetan los límites de tu servidor
- Elegir el estilo de nombre de archivo y la caducidad predeterminados

**Privacidad**

La extensión solo se comunica con el servidor que indiques. No incluye analíticas ni seguimiento y solo envía algo cuando usas una de sus acciones. No pide acceso a ningún sitio al instalarse, solo acceso a tu servidor cuando guardas la conexión.

**Qué necesitas**

- Un servidor de Fileship 6.0 o posterior: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Un token de subida de Configuración → Integraciones en tu panel de Fileship

Disponible en inglés, turco, alemán, español, francés, italiano, portugués de Brasil, ruso, azerí, japonés, coreano y chino simplificado. Código abierto con licencia MIT.
```

### Français (fr)

```text
Importez des fichiers, des images et des captures d'écran sur votre propre serveur Fileship et raccourcissez des liens, directement depuis votre navigateur.

Fileship est une plateforme open source et auto-hébergée pour les fichiers, les notes et les liens courts. Cette extension se connecte à votre serveur Fileship avec votre jeton d'importation.

**Ce que vous pouvez faire**

- Importer des fichiers depuis le bouton de la barre d'outils, ou les déposer sur la page d'importation
- Capturer la zone visible d'un onglet, ou faire glisser sur la page pour capturer une zone
- Faire un clic droit sur une image pour l'importer, ou sur un lien pour le raccourcir
- Raccourcir l'adresse de l'onglet actif
- Le lien est copié dans le presse-papiers et affiché dans une notification
- Suivre vos importations depuis le bouton de la barre d'outils, avec la progression, des boutons de copie et des messages d'erreur clairs
- Les gros fichiers sont importés en plusieurs parties en parallèle, selon les limites de votre serveur
- Choisir le style de nom de fichier et l'expiration par défaut

**Confidentialité**

L'extension ne communique qu'avec le serveur que vous indiquez. Elle ne contient ni statistiques ni suivi et n'envoie rien tant que vous n'utilisez pas l'une de ses actions. À l'installation, elle ne demande l'accès à aucun site ; l'accès à votre serveur n'est demandé qu'à l'enregistrement de la connexion.

**Prérequis**

- Un serveur Fileship 6.0 ou plus récent : [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Un jeton d'importation, depuis Paramètres → Intégrations dans votre tableau de bord Fileship

Disponible en anglais, turc, allemand, espagnol, français, italien, portugais du Brésil, russe, azerbaïdjanais, japonais, coréen et chinois simplifié. Open source sous licence MIT.
```

### Italiano (it)

```text
Carica file, immagini e screenshot sul tuo server Fileship e abbrevia i link, direttamente dal browser.

Fileship è una piattaforma open source e self-hosted per file, note e link brevi. Questa estensione si collega al tuo server Fileship con il tuo token di caricamento.

**Cosa puoi fare**

- Caricare file dal pulsante nella barra degli strumenti o trascinarli nella pagina di caricamento
- Acquisire l'area visibile di una scheda o trascinare sulla pagina per acquisire un'area
- Fare clic destro su un'immagine per caricarla o su un link per abbreviarlo
- Abbreviare l'indirizzo della scheda attiva
- Il link viene copiato negli appunti e mostrato in una notifica
- Seguire i caricamenti dal pulsante nella barra degli strumenti, con avanzamento, pulsanti per copiare e messaggi di errore chiari
- I file grandi vengono caricati in parti parallele, secondo i limiti del tuo server
- Scegliere lo stile predefinito del nome file e la scadenza

**Privacy**

L'estensione comunica solo con il server che inserisci. Non contiene statistiche né tracciamento e invia qualcosa solo quando usi una delle sue azioni. All'installazione non chiede l'accesso ad alcun sito: l'accesso al tuo server viene chiesto solo quando salvi la connessione.

**Requisiti**

- Un server Fileship 6.0 o successivo: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Un token di caricamento da Impostazioni → Integrazioni nella dashboard di Fileship

Disponibile in inglese, turco, tedesco, spagnolo, francese, italiano, portoghese brasiliano, russo, azero, giapponese, coreano e cinese semplificato. Open source con licenza MIT.
```

### Português (Brasil) (pt_BR)

```text
Envie arquivos, imagens e capturas de tela para o seu próprio servidor Fileship e encurte links, direto do navegador.

O Fileship é uma plataforma de código aberto e auto-hospedada para arquivos, notas e links curtos. Esta extensão se conecta ao seu servidor Fileship com o seu token de envio.

**O que você pode fazer**

- Enviar arquivos pelo botão da barra de ferramentas ou soltá-los na página de envio
- Capturar a parte visível de uma aba ou arrastar sobre a página para capturar uma área
- Clicar com o botão direito em uma imagem para enviá-la ou em um link para encurtá-lo
- Encurtar o endereço da aba atual
- O link é copiado para a área de transferência e mostrado em uma notificação
- Acompanhar os envios pelo botão da barra de ferramentas, com o progresso, botões de copiar e mensagens de erro claras
- Arquivos grandes são enviados em partes paralelas que respeitam os limites do seu servidor
- Escolher o estilo padrão do nome de arquivo e a expiração

**Privacidade**

A extensão se comunica apenas com o servidor que você informar. Ela não tem análises nem rastreamento e só envia algo quando você usa uma das suas ações. Na instalação, não pede acesso a nenhum site, apenas ao seu servidor quando você salva a conexão.

**O que você precisa**

- Um servidor Fileship 6.0 ou mais recente: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Um token de envio em Configurações → Integrações no painel do Fileship

Disponível em inglês, turco, alemão, espanhol, francês, italiano, português do Brasil, russo, azerbaijano, japonês, coreano e chinês simplificado. Código aberto sob a licença MIT.
```

### Русский (ru)

```text
Загружайте файлы, изображения и скриншоты на свой сервер Fileship и сокращайте ссылки прямо из браузера.

Fileship — платформа с открытым исходным кодом для файлов, заметок и коротких ссылок, которую вы размещаете на своём сервере. Расширение подключается к вашему серверу Fileship по токену загрузки.

**Что можно делать**

- Загружать файлы кнопкой на панели инструментов или перетаскивать их на страницу загрузки
- Делать скриншот видимой части вкладки или выделять на странице область для захвата
- Загружать изображение или сокращать ссылку из контекстного меню
- Сокращать адрес текущей вкладки
- Ссылка копируется в буфер обмена и показывается в уведомлении
- Следить за загрузками с кнопки на панели инструментов: прогресс, кнопки копирования и понятные сообщения об ошибках
- Большие файлы загружаются параллельными частями с учётом ограничений вашего сервера
- Выбирать стиль имени файла и срок действия по умолчанию

**Конфиденциальность**

Расширение обращается только к указанному вами серверу. В нём нет аналитики и отслеживания, и оно отправляет данные, только когда вы используете одно из его действий. При установке оно не запрашивает доступ к сайтам — только доступ к вашему серверу, когда вы сохраняете подключение.

**Что нужно**

- Сервер Fileship версии 6.0 или новее: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Токен загрузки из раздела «Настройки → Интеграции» в панели управления Fileship

Доступно на английском, турецком, немецком, испанском, французском, итальянском, бразильском португальском, русском, азербайджанском, японском, корейском и упрощённом китайском. Открытый исходный код под лицензией MIT.
```

### Azərbaycanca (az, AMO only)

```text
Faylları, şəkilləri və ekran görüntülərini öz Fileship serverinə yüklə və keçidləri qısalt, birbaşa brauzerindən.

Fileship fayllar, qeydlər və qısa keçidlər üçün açıq mənbəli, öz serverində işləyən platformadır. Bu genişləndirmə yükləmə açarınla Fileship serverinə qoşulur.

**Nələr edə bilərsən**

- Alətlər panelindəki düymədən fayl yüklə və ya faylları yükləmə səhifəsinə burax
- Tabın görünən sahəsini çək və ya səhifədə sürükləyərək bir sahə seç
- Şəklə sağ klikləyib yüklə, keçidə sağ klikləyib qısalt
- Açıq tabın ünvanını qısalt
- Keçid mübadilə buferinə kopyalanır və bildirişdə göstərilir
- Yükləmələrini alətlər panelindəki düymədən izlə: irəliləyiş, kopyalama düymələri və aydın xəta mesajları
- Böyük fayllar serverinin limitlərinə uyğun hissələrlə paralel yüklənir
- Standart fayl adı üslubunu və bitmə müddətini seç

**Məxfilik**

Genişləndirmə yalnız daxil etdiyin serverlə əlaqə saxlayır. Analitika və izləmə yoxdur və o, yalnız sən onun əməliyyatlarından birini istifadə etdikdə nəsə göndərir. Quraşdırılarkən heç bir sayta giriş istəmir, serverinə girişi isə yalnız bağlantını yadda saxladıqda istəyir.

**Nə lazımdır**

- Fileship 6.0 və ya daha yeni server: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Fileship idarə panelində Ayarlar → İnteqrasiyalar səhifəsindən yükləmə açarı

İngilis, türk, alman, ispan, fransız, italyan, Braziliya portuqal, rus, Azərbaycan, yapon, koreya və sadələşdirilmiş Çin dillərində mövcuddur. MIT lisenziyası ilə açıq mənbəli.
```

### 日本語 (ja)

```text
ファイル、画像、スクリーンショットを自分の Fileship サーバーにアップロードし、リンクを短縮できます。すべてブラウザーから直接操作できます。

Fileship は、ファイル、ノート、短縮リンクのためのオープンソースのセルフホスト型プラットフォームです。この拡張機能は、アップロードトークンを使って Fileship サーバーに接続します。

**できること**

- ツールバーのボタンからファイルをアップロード、またはアップロードページにドロップ
- タブの表示範囲をキャプチャ、またはページ上をドラッグして範囲をキャプチャ
- 画像を右クリックしてアップロード、リンクを右クリックして短縮
- 現在のタブのアドレスを短縮
- リンクはクリップボードにコピーされ、通知に表示されます
- ツールバーのボタンでアップロードを確認：進行状況、コピーボタン、わかりやすいエラーメッセージ
- 大きなファイルは、サーバーの制限に合わせて分割し、並列でアップロード
- 既定のファイル名の形式と有効期限を選択

**プライバシー**

拡張機能が通信するのは、入力したサーバーだけです。分析やトラッキングはなく、拡張機能の操作を使ったときにだけ送信します。インストール時にはどのサイトへのアクセスも求めず、サーバーへのアクセスは接続を保存したときにだけ求めます。

**必要なもの**

- Fileship 6.0 以降のサーバー：[github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Fileship のダッシュボードの「設定」→「連携」で取得できるアップロードトークン

英語、トルコ語、ドイツ語、スペイン語、フランス語、イタリア語、ポルトガル語（ブラジル）、ロシア語、アゼルバイジャン語、日本語、韓国語、簡体字中国語に対応しています。MIT ライセンスのオープンソースです。
```

### 한국어 (ko)

```text
파일, 이미지, 스크린샷을 내 Fileship 서버에 업로드하고 링크를 단축해요. 모두 브라우저에서 바로 할 수 있어요.

Fileship은 파일, 노트, 단축 링크를 위한 오픈 소스 셀프 호스팅 플랫폼이에요. 이 확장 프로그램은 업로드 토큰으로 Fileship 서버에 연결해요.

**할 수 있는 일**

- 툴바 버튼으로 파일을 업로드하거나 업로드 페이지에 끌어다 놓기
- 탭의 보이는 영역을 캡처하거나 페이지에서 드래그해 영역 캡처
- 이미지를 마우스 오른쪽 버튼으로 클릭해 업로드하고, 링크는 단축
- 현재 탭의 주소 단축
- 링크는 클립보드에 복사되고 알림으로 표시돼요
- 툴바 버튼에서 업로드 진행 상황, 복사 버튼, 알기 쉬운 오류 메시지 확인
- 큰 파일은 서버 제한에 맞춰 나눠서 동시에 업로드
- 기본 파일 이름 형식과 만료 기간 선택

**개인정보 보호**

확장 프로그램은 입력한 서버와만 통신해요. 분석이나 추적 기능이 없고, 확장 프로그램의 기능을 사용할 때만 데이터를 보내요. 설치할 때는 어떤 사이트에 대한 접근 권한도 요청하지 않고, 연결을 저장할 때만 서버 접근 권한을 요청해요.

**필요한 것**

- Fileship 6.0 이상 서버: [github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- Fileship 대시보드의 설정 → 연동에서 받은 업로드 토큰

영어, 터키어, 독일어, 스페인어, 프랑스어, 이탈리아어, 포르투갈어(브라질), 러시아어, 아제르바이잔어, 일본어, 한국어, 중국어(간체)를 지원해요. MIT 라이선스의 오픈 소스예요.
```

### 简体中文 (zh_CN)

```text
将文件、图片和截图上传到你自己的 Fileship 服务器，并缩短链接，全部在浏览器中完成。

Fileship 是一个开源、自托管的平台，用于管理文件、笔记和短链接。此扩展使用你的上传令牌连接到你的 Fileship 服务器。

**你可以**

- 通过工具栏按钮上传文件，或将文件拖放到上传页面
- 截取标签页的可见区域，或在页面上拖动以截取某个区域
- 右键点击图片即可上传，右键点击链接即可缩短
- 缩短当前标签页的地址
- 链接会复制到剪贴板，并显示在通知中
- 在工具栏按钮中查看上传进度、复制按钮和清晰的错误信息
- 大文件会按照服务器的限制分块并行上传
- 选择默认的文件名样式和过期时间

**隐私**

扩展只与你填写的服务器通信。它没有任何统计或跟踪功能，只有在你使用它的某项功能时才会发送数据。安装时不请求访问任何网站，只有在你保存连接时才请求访问你的服务器。

**你需要**

- Fileship 6.0 或更高版本的服务器：[github.com/hanzydev/Fileship](https://github.com/hanzydev/Fileship)
- 在 Fileship 控制台的“设置 → 集成”中获取的上传令牌

支持英语、土耳其语、德语、西班牙语、法语、意大利语、巴西葡萄牙语、俄语、阿塞拜疆语、日语、韩语和简体中文。基于 MIT 许可证开源。
```
