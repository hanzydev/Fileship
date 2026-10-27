<div align="center">
  <a href="https://github.com/hanzydev/Fileship">
    <img src="banner.png" alt="Fileship" width="520" height="200">
  </a>

  <p align="center">
    나만의 ShareX 서버이자 파일 클라우드예요. 네이티브 Windows 앱, 파일 탐색기 동기화, 내 서버에서 실행되는 AI 검색, 브라우저 확장 프로그램을 제공해요.
    <br />
    <br />
    <a href="https://github.com/hanzydev/fileship-desktop/releases/latest">Windows용 다운로드</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=bug-report.yml">버그 신고</a>
    ·
    <a href="https://github.com/hanzydev/Fileship/issues/new?template=feature-request.yml">기능 제안</a>
    ·
    <a href="SECURITY.md">보안 정책</a>
  </p>

  <p align="center">
    <a href="README.md">English</a> · <a href="README.tr.md">Türkçe</a> · <a href="README.de.md">Deutsch</a> · <a href="README.es.md">Español</a> · <a href="README.fr.md">Français</a> · <a href="README.it.md">Italiano</a> · <a href="README.pt-BR.md">Português (Brasil)</a> · <a href="README.ru.md">Русский</a> · <a href="README.az.md">Azərbaycanca</a> · <a href="README.ja.md">日本語</a> · <b>한국어</b> · <a href="README.zh-CN.md">简体中文</a>
  </p>
</div>

<p align="center">
  <img src="docs/images/demo.gif" alt="Fileship에 이미지를 끌어다 놓으면 업로드되고 링크가 복사되며, 스마트 검색이 문장으로 사진을 찾아 AI 설명을 보여 주는 모습" width="100%">
</p>

## 📦 기능

- 파일, 다른 사용자와 공유할 수 있는 중첩 폴더, Markdown 노트, 단축 링크, 받은 파일함, 그리고 삭제한 항목이 모이는 휴지통
- 직접 또는 예약으로 만들고, 다운로드하고 복원할 수 있는 계정 백업
- 비밀번호, 만료일, 조회수 제한을 설정할 수 있는 공개 공유와 Discord 등의 앱에 표시되는 풍부한 링크 미리보기
- 내 서버에서 실행되는 [AI](docs/ai.md): 50개 이상의 언어를 지원하는 스마트 검색, 설명, OCR, 민감한 콘텐츠 감지(모두 선택 사항)
- 로컬 디스크, S3 호환 서비스, SFTP, FTP 중 원하는 곳에 업로드 저장
- 권한과 저장 용량 제한을 둘 수 있는 다중 사용자, 패스키, 2단계 인증, OpenID Connect 기반 [싱글 사인온](docs/configuration.md#single-sign-on)
- 다운타임 없는 업데이트, 클릭 한 번으로 또는 예약해서
- ShareX, macOS, Linux 업로드 연동, [Windows 앱](docs/desktop.md), Android 앱에서 공유할 수 있는 [휴대폰용 설치형 앱](docs/mobile.md)
- 파일, 이미지, 스크린샷을 업로드하고 링크를 단축하는 [브라우저 확장 프로그램](docs/browser-extension.md). Chrome과 Edge용은 [Chrome 웹 스토어](https://chromewebstore.google.com/detail/fileship/lbnlfnmlaipoflalhikladilalkoagnh)에 있고, Firefox용은 [Firefox 부가 기능](https://addons.mozilla.org/firefox/addon/fileship/)에 있어요
- Discord, Slack, 직접 만든 서비스로 보내는 [웹훅](docs/webhooks.md)
- 영어, 터키어, 독일어, 스페인어, 프랑스어, 이탈리아어, 브라질 포르투갈어, 러시아어, 아제르바이잔어, 일본어, 한국어, 중국어 간체

## 🆚 다른 프로젝트와 비교

|                                   | Fileship                       | [Zipline](https://github.com/diced/zipline) | [XBackBone](https://github.com/sergix44/XBackBone) | [Chibisafe](https://github.com/chibisafe/chibisafe) |
| --------------------------------- | ------------------------------ | ------------------------------------------- | -------------------------------------------------- | --------------------------------------------------- |
| ShareX 및 기타 업로더             | ✅                             | ✅                                          | ✅                                                 | ✅                                                  |
| 캡처와 녹화를 위한 데스크톱 앱    | ✅ Windows                     | ❌                                          | ❌                                                 | ❌                                                  |
| 파일 탐색기에서 내 파일 보기      | ✅ Fileship Harbor             | ❌                                          | ❌                                                 | ❌                                                  |
| 브라우저 확장 프로그램            | ✅                             | ❌                                          | ❌                                                 | ✅                                                  |
| AI 검색, 설명, OCR, 민감한 콘텐츠 | ✅ 내 서버에서 실행            | ❌                                          | ❌                                                 | ❌                                                  |
| 단축 링크                         | ✅                             | ✅                                          | ❌                                                 | ✅                                                  |
| 노트 또는 텍스트 조각             | ✅ Markdown                    | ✅                                          | 🟡 텍스트 업로드                                   | ✅                                                  |
| 폴더                              | ✅ 중첩, 사용자와 공유         | ✅ 중첩                                     | ❌ 태그                                            | 🟡 단일 계층 앨범                                   |
| 비밀번호, 만료, 조회수 제한       | ✅                             | ✅                                          | ❌                                                 | 🟡 앨범 링크 만료                                   |
| 저장소                            | 로컬, S3, SFTP, FTP            | 로컬, S3                                    | 로컬, S3, FTP, Dropbox, Google Cloud               | 로컬, S3                                            |
| 2단계 인증과 패스키               | ✅                             | ✅                                          | ❌                                                 | ❌                                                  |
| 싱글 사인온                       | ✅ OIDC                        | ✅ OAuth, OIDC                              | ✅ LDAP                                            | ❌                                                  |
| 웹훅                              | ✅ Discord, Slack, 서명된 JSON | ✅ Discord, HTTP                            | ❌                                                 | ❌                                                  |
| 복원 가능한 계정 백업, 예약 지원  | ✅                             | 🟡 내보내기                                 | 🟡 내보내기                                        | ❌                                                  |
| 대시보드에서 업데이트             | ✅ 다운타임 없음               | ❌                                          | 🟡 다운타임 있음                                   | ❌                                                  |
| 언어                              | 12                             | 12                                          | 31, 일부는 부분 번역                               | 1                                                   |
| 라이선스                          | MIT                            | MIT                                         | AGPL-3.0                                           | MIT                                                 |

2026년 10월 기준 Zipline 4.8, XBackBone 3.8, Chibisafe 6.7과 비교했어요. 오래된 정보가 있나요? [이슈를 열어](https://github.com/hanzydev/Fileship/issues/new/choose) 주시면 고칠게요.

## 📸 스크린샷

<p align="center">
  <img src="apps/dashboard/public/previews/fileship-desktop.png" alt="데스크톱에서 본 Fileship 대시보드" width="78%">
  &nbsp;
  <img src="apps/dashboard/public/previews/fileship-mobile.png" alt="모바일에서 본 Fileship 파일 목록" width="18%">
</p>

## 🖥️ Windows용 Fileship

알림 영역에서 ShareX처럼 캡처하고 녹화해서 Fileship에 업로드하고, Fileship Harbor로 파일 탐색기에서 내 파일을 둘러볼 수 있어요. 64비트 Windows 11 버전 24H2 이상과 Fileship 6.0 서버가 필요해요.

**[최신 버전 다운로드](https://github.com/hanzydev/fileship-desktop/releases/latest)** · [기능 및 설정 방법](docs/desktop.md)

## 🚀 시작하기

[Docker](https://www.docker.com/)와 [Docker Compose](https://docs.docker.com/compose/), CPU 2코어, RAM 2GB가 필요해요(AI를 쓰려면 4GB 이상, [요구 사항](docs/installation.md#requirements) 참고).

```sh
git clone https://github.com/hanzydev/Fileship.git
cd Fileship
echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env
docker compose up -d
```

이제 `http://localhost:3000`에서 Fileship을 쓸 수 있어요. `admin` / `password`로 로그인하세요.

> [!IMPORTANT]
> 처음 로그인한 뒤 바로 비밀번호를 바꾸세요(**설정 → 보안**).

5.x에서 업그레이드하나요? [6.0으로 업그레이드하기](docs/upgrading.md)를 따라 하세요. Zipline, XBackBone, Chibisafe를 쓰고 있었나요? [계정, 파일, 링크를 그대로 옮겨 올 수 있어요](docs/migrating.md).

## 📚 문서

- [설치](docs/installation.md): 요구 사항, 지원 브라우저, 내 도메인에서 Fileship 운영하기
- [구성](docs/configuration.md): 서버 설정, 저장소, 휴지통, 환경 변수
- [업데이트](docs/updating.md): 업데이트 방식과 예약 방법
- [다른 앱에서 옮겨 오기](docs/migrating.md): Zipline, XBackBone, Chibisafe 서버 가져오기
- [AI 기능](docs/ai.md): 프리셋, 모델, 스마트 검색, 민감한 콘텐츠
- [Windows용 Fileship](docs/desktop.md)
- [휴대폰에서 Fileship 쓰기](docs/mobile.md): 앱 설치와 Android에서 공유하기
- [웹훅](docs/webhooks.md): 이벤트, 형식, 전송, 서명 검증
- [원격 분석](docs/telemetry.md): 수집하는 정보와 끄는 방법

## 🧰 사용 기술

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

## ❤️ 기여하기

기여는 언제나 환영해요. 먼저 [기여 가이드라인](CONTRIBUTING.md)을 읽어 주세요. 코드베이스의 구조와 규칙은 [AGENTS.md](AGENTS.md)에 정리되어 있어요.

## ⚖️ 라이선스

[MIT](LICENSE)
