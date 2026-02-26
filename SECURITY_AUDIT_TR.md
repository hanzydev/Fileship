# Fileship Güvenlik ve Bug Taraması (Özet Rapor)

Tarih: 2026-02-26
Kapsam: `apps/dashboard` backend + auth + backup + dosya erişim akışları
Yöntem: Statik kod inceleme (manuel), pattern taraması ve temel komut satırı kontrolleri

## Kritik Bulgular

### 1) `findCurrentSessionById!` null-assertion kaynaklı servis hatası (DoS vektörü)
- Dosya: `apps/dashboard/server/utils/verifySession.ts`
- Problem:
  - `findCurrentSessionById!` non-null assertion ile kullanılıyor.
  - Session kaydı yoksa runtime crash / 500 üretebilir.
- Risk: Hata tetiklenerek endpoint istikrarı düşürülebilir.
- Öneri:
  - Session yoksa kontrollü `401/403` dönün; asla non-null assertion ile devam etmeyin.

### 2) Oturum çerezi güvenlik bayrakları eksik (`httpOnly`, `secure`)
- Dosyalar:
  - `apps/dashboard/server/api/auth/login/index.post.ts`
  - `apps/dashboard/server/api/auth/login/passwordless.post.ts`
- Problem:
  - `setCookie` çağrılarında yalnızca `sameSite: 'lax'` var.
  - `httpOnly` ve `secure` ayarlanmamış.
- Risk:
  - XSS durumunda cookie JavaScript tarafından okunabilir.
  - HTTPS zorlaması yoksa ağ düzeyinde risk artar.
- Öneri:
  - `httpOnly: true`, `secure: true` (prod), mümkünse `sameSite: 'strict'` değerlendirin.

### 3) Dosya paylaşım parolası düz metin saklanıyor ve düz metin karşılaştırılıyor
- Dosyalar:
  - `apps/dashboard/server/api/files/index.post.ts`
  - `apps/dashboard/server/routes/u/[id]/index.get.ts`
- Problem:
  - Upload sırasında parola `password: body.data.password` olarak DB'ye direkt yazılıyor.
  - Erişimde `query.password !== findFileById.password` ile düz metin kıyaslanıyor.
- Risk:
  - Veritabanı sızıntısında tüm paylaşım parolaları ifşa olur.
- Öneri:
  - Parolaları Argon2/bcrypt ile hashleyin.
  - Doğrulamada hash karşılaştırması kullanın; mevcut tasarımla uyumlu olacak şekilde query-string akışını koruyup sadece plaintext karşılaştırmayı kaldırın.
  - Geriye dönük uyumluluk için migration taskı planlayın (mevcut plaintext kayıtları tek seferde hash'leme / ilk kullanımda dönüştürme stratejisi).

## Yüksek Bulgular

### 4) Backup restore akışında arşiv çıkarma sırasında path traversal riski
- Dosya: `apps/dashboard/server/api/users/@me/backups/[id]/load.post.ts`
- Problem:
  - `extract({ file: backupPath, cwd: tempPath })` çağrısında path traversal koruması görünmüyor.
- Risk:
  - Kötücül `.tgz` içerikleri hedef dizin dışına yazmaya çalışabilir.
- Öneri:
  - Arşiv entry path doğrulaması yapın (`..`, absolute path, symlink) ve güvenli extract politikası uygulayın.

### 5) Backup içeriği ile kullanıcı üzerinde yetki/esas alanların toplu güncellenmesi
- Dosya: `apps/dashboard/server/api/users/@me/backups/[id]/load.post.ts`
- Problem:
  - `prisma.user.update({ data: userData })` ile yedekten gelen alanlar doğrudan uygulanıyor.
- Risk:
  - Kötücül backup ile `superAdmin`, `permissions`, güvenlik ayarları gibi alanların manipülasyonu mümkün olabilir.
- Öneri:
  - Sadece izinli alanları (allowlist) update edin.
  - Yetki alanları ve güvenlik-kritik alanları restore kapsamı dışında bırakın.

## Orta Bulgular

### 6) Dosya yüklemede `folderId` sahiplik kontrolü eksik
- Dosya: `apps/dashboard/server/api/files/index.post.ts`
- Problem:
  - `folderId` varlığında sadece `findUnique(id)` kontrolü var; klasörün kullanıcıya ait olup olmadığı doğrulanmıyor.
- Risk:
  - Kullanıcılar başka kullanıcı klasör ID'lerini kullanarak veri ilişkilendirmesi yapabilir.
- Öneri:
  - `findUnique` yerine `findFirst` + `where: { id: folderId, authorId: currentUser.id }` veya eşdeğer bir sorgu ile sahiplik doğrulayın.

## Operasyonel Not
- `pnpm audit --prod` komutu npm audit endpoint tarafında `403 Forbidden` döndüğü için bağımlılık CVE listesi otomatik çekilemedi.

## Önceliklendirilmiş Aksiyon Planı
1. Session cookie bayraklarını sertleştirin (`httpOnly`, `secure`) (kritik).
2. Dosya parola saklamayı Argon2 ile hash'e taşıyın + migration taskı ekleyin (kritik).
3. Backup restore için güvenli extract + user update allowlist uygulayın (yüksek).
4. `folderId` sahiplik kontrolünü `authorId` ile zorunlu doğrulayın (orta).
