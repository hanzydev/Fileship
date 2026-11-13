import { fork } from 'node:child_process';
import { mkdir, readFile, rm, writeFile } from 'node:fs/promises';
import { createRequire } from 'node:module';
import { join } from 'node:path';

const root = join(import.meta.dirname, '..', '..', '..');
const fixtures = join(import.meta.dirname, 'fixtures');
const require = createRequire(join(root, 'packages', 'ai', 'package.json'));
const sharp = require('sharp');

const THREADS = 2;
const FONT = 'Segoe UI, Malgun Gothic, Yu Gothic, Microsoft YaHei, Tahoma, Noto Sans, sans-serif';
const SERIF = 'Georgia, Times New Roman, serif';

const MSIGLIP = {
    key: 'msiglip-b16',
    id: 'pulsejet/siglip-base-patch16-256-multilingual-onnx',
    visionDtype: 'q8',
};

const MINILM = {
    key: 'minilm-l12',
    id: 'Xenova/paraphrase-multilingual-MiniLM-L12-v2',
    dtype: 'q8',
    maxChars: 2_000,
};

const PRESETS = {
    low: {
        image: { ...MSIGLIP, textDtype: 'q8' },
        text: MINILM,
        caption: { task: '<CAPTION>', visionDtype: 'q8' },
        ocr: 'tiny',
        keepAliveMs: 2 * 60_000,
        videoFrames: 8,
    },
    medium: {
        image: { ...MSIGLIP, textDtype: 'fp32' },
        text: MINILM,
        caption: { task: '<DETAILED_CAPTION>', visionDtype: 'q8' },
        ocr: 'small',
        keepAliveMs: 5 * 60_000,
        videoFrames: 16,
    },
    high: {
        image: { ...MSIGLIP, textDtype: 'fp32' },
        text: MINILM,
        caption: { task: '<DETAILED_CAPTION>', visionDtype: 'fp32' },
        ocr: 'small',
        keepAliveMs: 15 * 60_000,
        videoFrames: 32,
    },
};

const random = (seed) => () => {
    seed = (seed + 0x6d2b79f5) | 0;
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;

    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
};

const escape = (text) => text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');

const svg = (width, height, body) =>
    Buffer.from(
        `<svg xmlns="http://www.w3.org/2000/svg" width="${width}" height="${height}">${body}</svg>`,
    );

const textLines = (lines, { x, y, size, gap, font = FONT, fill = '#111', anchor = 'start' }) =>
    lines
        .map(
            (line, i) =>
                `<text x="${x}" y="${y + i * gap}" font-family="${font}" font-size="${size}" fill="${fill}" text-anchor="${anchor}">${escape(line)}</text>`,
        )
        .join('');

const SCREENSHOT_TEXT = [
    'Files',
    'Folders',
    'Notes',
    'Quarterly report',
    'Revenue grew by 12 percent',
    'Uploads this week: 1,284',
];

const DOCUMENT_TEXT = [
    'Toplantı notları',
    'Bütçe cuma günü gözden geçirilecek',
    'Tasarım ekibiyle görüşme saat 14:30',
    'Teslim tarihi: 28 Kasım 2026',
];

const scenes = {
    sunset: () => {
        const waves = Array.from(
            { length: 9 },
            (_, i) =>
                `<rect x="${330 - i * 12}" y="${400 + i * 20}" width="${140 + i * 24}" height="4" fill="#ffd27f" opacity="${0.8 - i * 0.07}"/>`,
        ).join('');

        return svg(
            800,
            600,
            `<defs><linearGradient id="sky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#3b1e5c"/><stop offset="0.6" stop-color="#f0645a"/><stop offset="1" stop-color="#ffb35c"/></linearGradient><linearGradient id="sea" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#2a3d6b"/><stop offset="1" stop-color="#0d1630"/></linearGradient></defs><rect width="800" height="390" fill="url(#sky)"/><circle cx="400" cy="380" r="90" fill="#ffd34d"/><rect y="390" width="800" height="210" fill="url(#sea)"/>${waves}`,
        );
    },
    shapes: () =>
        svg(
            800,
            600,
            '<rect width="800" height="600" fill="#ffffff"/><circle cx="190" cy="300" r="120" fill="#e53935"/><rect x="330" y="190" width="220" height="220" fill="#1e63d6"/><polygon points="680,170 790,420 570,420" fill="#2e9e44"/>',
        ),
    chart: () => {
        const values = [120, 260, 180, 330, 290, 410];
        const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun'];
        const bars = values
            .map(
                (value, i) =>
                    `<rect x="${120 + i * 105}" y="${520 - value}" width="70" height="${value}" fill="#4f7cff"/><text x="${155 + i * 105}" y="550" font-family="${FONT}" font-size="20" fill="#333" text-anchor="middle">${months[i]}</text>`,
            )
            .join('');

        return svg(
            800,
            600,
            `<rect width="800" height="600" fill="#ffffff"/><text x="400" y="60" font-family="${FONT}" font-size="30" fill="#111" text-anchor="middle">Monthly sales</text><line x1="90" y1="520" x2="760" y2="520" stroke="#444" stroke-width="2"/><line x1="90" y1="90" x2="90" y2="520" stroke="#444" stroke-width="2"/>${bars}`,
        );
    },
    screenshot: () =>
        svg(
            960,
            600,
            `<rect width="960" height="600" fill="#f4f5f7"/><rect width="960" height="44" fill="#e3e5e8"/><circle cx="24" cy="22" r="7" fill="#ff5f57"/><circle cx="46" cy="22" r="7" fill="#febc2e"/><circle cx="68" cy="22" r="7" fill="#28c840"/><rect y="44" width="220" height="556" fill="#ffffff"/>${textLines(SCREENSHOT_TEXT.slice(0, 3), { x: 32, y: 104, size: 22, gap: 52, fill: '#333' })}<rect x="260" y="80" width="660" height="480" rx="14" fill="#ffffff"/>${textLines(SCREENSHOT_TEXT.slice(3, 4), { x: 300, y: 150, size: 36, gap: 0 })}${textLines(SCREENSHOT_TEXT.slice(4), { x: 300, y: 220, size: 24, gap: 48, fill: '#444' })}<rect x="300" y="340" width="420" height="16" rx="8" fill="#dfe3ea"/><rect x="300" y="340" width="260" height="16" rx="8" fill="#4f7cff"/>`,
        ),
    document: () => {
        const rand = random(7);
        const grain = Array.from(
            { length: 220 },
            () =>
                `<rect x="${Math.floor(rand() * 900)}" y="${Math.floor(rand() * 700)}" width="${80 + Math.floor(rand() * 220)}" height="2" fill="#5a3518" opacity="0.35"/>`,
        ).join('');

        return svg(
            900,
            700,
            `<rect width="900" height="700" fill="#8a5a2e"/>${grain}<g transform="rotate(-4 450 350) skewX(3)"><rect x="150" y="90" width="600" height="520" fill="#fbfaf5"/><rect x="150" y="610" width="600" height="8" fill="#000" opacity="0.25"/>${textLines(DOCUMENT_TEXT, { x: 190, y: 180, size: 30, gap: 70, font: SERIF })}</g>`,
        );
    },
    city: () => {
        const rand = random(3);
        const stars = Array.from(
            { length: 60 },
            () =>
                `<circle cx="${Math.floor(rand() * 800)}" cy="${Math.floor(rand() * 260)}" r="${1 + rand() * 1.5}" fill="#fff"/>`,
        ).join('');
        const buildings = Array.from({ length: 9 }, (_, i) => {
            const height = 180 + Math.floor(rand() * 260);
            const x = i * 90;
            const windows = Array.from({ length: 24 }, (_, w) =>
                rand() > 0.45
                    ? `<rect x="${x + 12 + (w % 4) * 18}" y="${600 - height + 20 + Math.floor(w / 4) * 30}" width="10" height="16" fill="#ffd966"/>`
                    : '',
            ).join('');

            return `<rect x="${x}" y="${600 - height}" width="84" height="${height}" fill="#1b2233"/>${windows}`;
        }).join('');

        return svg(
            800,
            600,
            `<rect width="800" height="600" fill="#070b1a"/>${stars}<circle cx="660" cy="90" r="38" fill="#f4f1d0"/>${buildings}`,
        );
    },
    forest: () => {
        const trees = Array.from({ length: 14 }, (_, i) => {
            const x = 20 + i * 57;
            const top = 260 + (i % 3) * 30;

            return `<rect x="${x + 20}" y="${top + 150}" width="14" height="60" fill="#5b3a1e"/><polygon points="${x + 27},${top} ${x + 70},${top + 160} ${x - 16},${top + 160}" fill="${i % 2 ? '#1f6b2e' : '#2b8a3e'}"/>`;
        }).join('');

        return svg(
            800,
            600,
            `<rect width="800" height="600" fill="#8fd3ff"/><ellipse cx="200" cy="520" rx="420" ry="200" fill="#4caf50"/><ellipse cx="640" cy="540" rx="380" ry="180" fill="#3e9b45"/>${trees}`,
        );
    },
    beach: () =>
        svg(
            800,
            600,
            '<rect width="800" height="600" fill="#87ceeb"/><rect y="260" width="800" height="120" fill="#1e88c7"/><rect y="380" width="800" height="220" fill="#f2d49b"/><rect x="398" y="300" width="8" height="230" fill="#6d4c41"/><path d="M290 310 Q402 190 514 310 Z" fill="#e53935"/><circle cx="660" cy="110" r="50" fill="#ffeb3b"/>',
        ),
};

const OCR_TEXT = {
    en: ['Invoice 2026-114', 'Total due: 1,250.00 USD', 'Thank you for your order'],
    tr: ['Fatura numarası 2026-114', 'Ödenecek tutar: 1.250,00 TL', 'Siparişiniz için teşekkürler'],
    de: ['Rechnung Nr. 2026-114', 'Gesamtbetrag: 1.250,00 €', 'Größe und Maße prüfen'],
    es: ['Factura n.º 2026-114', 'Importe total: 1.250,00 €', '¡Gracias por su pedido!'],
    fr: ['Facture n° 2026-114', 'Montant déjà réglé à l’échéance', 'Merci pour votre commande'],
    it: ['Fattura n. 2026-114', 'Importo totale: 1.250,00 €', 'Perché è già così?'],
    pt: ['Fatura nº 2026-114', 'Valor total: R$ 1.250,00', 'Informação sobre a ação'],
    ru: ['Счёт № 2026-114', 'Итого к оплате: 1 250,00 ₽', 'Спасибо за ваш заказ'],
    az: [
        'Faktura nömrəsi 2026-114',
        'Ödəniləcək məbləğ: 1.250,00 ₼',
        'Sifarişiniz üçün təşəkkürlər',
    ],
    ja: ['請求書番号 2026-114', 'お支払い金額：1,250円', 'ご注文ありがとうございます'],
    ko: ['청구서 번호 2026-114', '결제 금액: 1,250원', '주문해 주셔서 감사합니다'],
    zh: ['发票编号 2026-114', '应付金额：1,250.00 元', '感谢您的订购'],
    ar: ['فاتورة جديدة', 'المبلغ الإجمالي', 'شكرا لطلبك'],
    th: ['ใบแจ้งหนี้ใหม่', 'ยอดรวมทั้งหมด', 'ขอบคุณสำหรับการสั่งซื้อ'],
    el: ['Τιμολόγιο 2026-114', 'Συνολικό ποσό: 1.250,00 €', 'Ευχαριστούμε για την παραγγελία'],
};

const ROTATED_TEXT = ['The quick brown fox', 'jumps over the lazy dog', 'Order 2026-114 shipped'];
const TILTED_TEXT = ['Meeting notes', 'Budget review on Friday', 'Call the design team'];
const TERMINAL_TEXT = ['$ pnpm install', 'Done in 4.2s', 'Server listening on port 3000'];

const textImage = (lines, { fill = '#111', background = '#fff', font = FONT } = {}) =>
    svg(
        960,
        lines.length * 64 + 60,
        `<rect width="100%" height="100%" fill="${background}"/>${textLines(lines, { x: 40, y: 70, size: 34, gap: 64, fill, font, anchor: 'start' })}`,
    );

const rightToLeft = (lines) =>
    svg(
        960,
        lines.length * 64 + 60,
        `<rect width="100%" height="100%" fill="#fff"/>${textLines(lines, { x: 920, y: 70, size: 34, gap: 64, anchor: 'end' })}`,
    );

const ocrImages = {
    ...Object.fromEntries(
        Object.entries(OCR_TEXT).map(([lang, lines]) => [
            `text-${lang}`,
            {
                lines,
                render: () =>
                    sharp(lang === 'ar' ? rightToLeft(lines) : textImage(lines)).removeAlpha(),
            },
        ]),
    ),
    ...Object.fromEntries(
        [90, 180, 270].map((angle) => [
            `rotated-${angle}`,
            {
                lines: ROTATED_TEXT,
                render: async () =>
                    sharp(
                        await sharp(textImage(ROTATED_TEXT)).removeAlpha().png().toBuffer(),
                    ).rotate(angle),
            },
        ]),
    ),
    tilted: {
        lines: TILTED_TEXT,
        render: async () =>
            sharp(await sharp(textImage(TILTED_TEXT)).removeAlpha().png().toBuffer()).rotate(7, {
                background: '#ffffff',
            }),
    },
    terminal: {
        lines: TERMINAL_TEXT,
        render: () =>
            sharp(
                textImage(TERMINAL_TEXT, {
                    fill: '#d6e2ff',
                    background: '#1d2130',
                    font: 'Consolas, monospace',
                }),
            ).removeAlpha(),
    },
    screenshot: { lines: SCREENSHOT_TEXT, scene: true },
    document: { lines: DOCUMENT_TEXT, scene: true },
};

const QUERIES = [
    'sunset over the sea',
    'gün batımında deniz',
    'Sonnenuntergang am Meer',
    'puesta de sol en la playa',
    'coucher de soleil sur la mer',
    'tramonto sul mare',
    'pôr do sol no mar',
    'закат над морем',
    'dənizdə gün batımı',
    '海に沈む夕日',
    '바다 위의 일몰',
    '海上日落',
    'Red circle and BLUE square',
    'bar chart',
    'screenshot of an app',
    'document on a wooden desk',
    'city at night',
    'forest with green trees',
    'beach umbrella',
    'İstanbul’da kış',
    'グラフ',
    '숲속의 나무',
    'photo of the garden behind the old stone house in summer with roses, tulips, a wooden bench, a small fountain, two cats sleeping in the sun, a red bicycle leaning on the fence, apple trees, a blue door, children playing football and a dog chasing birds',
];

const LONG_ENGLISH = Array.from(
    { length: 40 },
    (_, i) =>
        `Paragraph ${i + 1} of the travel diary describes a quiet morning by the harbour, fresh bread from the bakery and boats leaving for the islands.`,
).join(' ');

const LONG_CHINESE = Array.from(
    { length: 60 },
    (_, i) => `第${i + 1}段：今天早上我们在港口散步，买了新鲜的面包，看着渔船出海。`,
).join('');

const TEXTS = [
    'The new storage server arrived today. We will move the photo archive this weekend and check every backup afterwards.',
    'Yeni depolama sunucusu bugün geldi. Fotoğraf arşivini hafta sonu taşıyıp bütün yedekleri kontrol edeceğiz.',
    'Der neue Speicherserver ist heute angekommen. Am Wochenende ziehen wir das Fotoarchiv um und prüfen alle Backups.',
    'El nuevo servidor de almacenamiento llegó hoy. Este fin de semana moveremos el archivo de fotos y revisaremos las copias.',
    'Le nouveau serveur de stockage est arrivé aujourd’hui. Nous déplacerons les photos ce week-end et vérifierons les sauvegardes.',
    'Il nuovo server di archiviazione è arrivato oggi. Questo fine settimana sposteremo le foto e controlleremo i backup.',
    'O novo servidor de armazenamento chegou hoje. Neste fim de semana vamos mover o arquivo de fotos e conferir os backups.',
    'Новый сервер хранения прибыл сегодня. На выходных мы перенесём фотоархив и проверим все резервные копии.',
    'Yeni yaddaş serveri bu gün gəldi. Həftə sonu foto arxivini köçürüb bütün ehtiyat nüsxələri yoxlayacağıq.',
    '新しいストレージサーバーが今日届きました。週末に写真のアーカイブを移して、すべてのバックアップを確認します。',
    '새 저장소 서버가 오늘 도착했어요. 주말에 사진 보관함을 옮기고 모든 백업을 확인할 거예요.',
    '新的存储服务器今天到了。我们周末会迁移照片存档，并检查所有备份。',
    LONG_ENGLISH,
    LONG_CHINESE,
    '  Lots   of\n\n\twhitespace here   and\r\nthere  ',
    'café 🙂 naïve résumé 𝔘𝔫𝔦𝔠𝔬𝔡𝔢 ﬁne',
    ' \n\t ',
];

const FIRST_NAMES = ['Jane', 'John', 'Ayşe', 'Mehmet', 'Lukas', 'Sofia', 'Chloé', 'Marco'];
const LAST_NAMES = ['Doe', 'Roe', 'Yılmaz', 'Kaya', 'Schmidt', 'García', 'Martin', 'Rossi'];

const longJson = () => {
    const rand = random(11);
    const records = [];

    while (JSON.stringify(records, null, 2).length < 18_500) {
        const first = FIRST_NAMES[Math.floor(rand() * FIRST_NAMES.length)];
        const last = LAST_NAMES[Math.floor(rand() * LAST_NAMES.length)];
        const id = records.length + 1;

        records.push({
            id,
            name: `${first} ${last}`,
            email: `${first}.${last}${id}@example.com`.toLowerCase(),
            phone: `+1 555 01${String(id).padStart(2, '0')}`,
            address: `${100 + id} Example Street, Springfield`,
            birthday: `19${70 + (id % 30)}-0${1 + (id % 9)}-1${id % 10}`,
            plan: id % 3 ? 'free' : 'pro',
            uploads: Math.floor(rand() * 900),
        });
    }

    return JSON.stringify(records, null, 2);
};

const PRIVACY_TEXTS = [
    'Contact Jane Doe at jane.doe@example.com or +1 555 0100. Her address is 742 Evergreen Terrace, Springfield.',
    'Ayşe Yılmaz, TC kimlik numarası 10000000146, telefon 0532 000 00 00, IBAN TR33 0006 1005 1978 6457 8413 26.',
    'Bitte überweisen Sie an Max Mustermann, IBAN DE89 3704 0044 0532 0130 00, geboren am 12.03.1985.',
    'Card number 4111 1111 1111 1111, expires 12/29, security code 123.',
    'api_key = sk_test_4eC39HqLyjWDarjtT1zdp7dc\npassword: hunter2-correct-horse',
    'The quarterly report shows revenue grew by 12 percent compared to last year.',
    'Écrivez à Chloé Martin, chloe.martin@example.fr, 06 12 34 56 78, 8 rue de la Paix, Paris.',
    'Llama a Sofía García al +34 600 000 000 o visita https://example.es/perfil/sofia.',
    'Иван Петров, тел. +7 900 000-00-00, адрес: ул. Ленина, д. 1, Москва.',
    '山田太郎さんの電話番号は090-0000-0000、メールはtaro@example.jpです。',
    '김민수 님의 연락처는 010-0000-0000이고 주소는 서울시 중구 세종대로 110입니다.',
    '请联系张伟，电话 138 0000 0000，邮箱 zhangwei@example.cn。',
    longJson(),
];

const render = async () => {
    await rm(fixtures, { recursive: true, force: true });
    await mkdir(fixtures, { recursive: true });

    for (const [name, scene] of Object.entries(scenes)) {
        let image = sharp(scene()).removeAlpha();

        if (name === 'document')
            image = sharp(await image.blur(0.8).jpeg({ quality: 55 }).toBuffer());

        await image.png({ compressionLevel: 9 }).toFile(join(fixtures, `${name}.png`));
    }

    for (const [name, { render: draw }] of Object.entries(ocrImages)) {
        if (!draw) continue;

        const image = await draw();
        await image.png({ compressionLevel: 9 }).toFile(join(fixtures, `${name}.png`));
    }

    const inputs = {
        scenes: Object.keys(scenes),
        ocr: Object.fromEntries(
            Object.entries(ocrImages).map(([name, { lines }]) => [name, lines.join('\n')]),
        ),
        queries: QUERIES,
        texts: TEXTS,
        privacy: PRIVACY_TEXTS,
    };

    await writeFile(join(fixtures, 'inputs.json'), JSON.stringify(inputs, null, 4));
    return inputs;
};

const encodeVector = (vector) => Buffer.from(new Float32Array(vector).buffer).toString('base64');

const runPreset = async (name, inputs) => {
    const { createAIClient } = await import(
        new URL(`file:///${join(root, 'packages', 'ai', 'dist', 'index.mjs')}`).href
    );
    const client = createAIClient({
        config: () => ({
            preset: PRESETS[name],
            threads: THREADS,
            cacheDir: join(root, 'data', 'models'),
        }),
        log: (message) => console.log(`[${name}] ${message}`),
    });

    let peakRss = 0;
    const sampler = setInterval(() => {
        peakRss = Math.max(peakRss, process.memoryUsage().rss);
    }, 50);

    const timings = {};
    const timed = async (method, fn) => {
        const start = performance.now();
        const value = await fn();

        (timings[method] ??= []).push(Math.round(performance.now() - start));
        return value;
    };

    const fixture = (file) => join(fixtures, `${file}.png`);
    const result = { images: {}, queries: {}, texts: [], privacy: [], captions: {}, ocr: {} };

    for (const scene of inputs.scenes)
        result.images[scene] = encodeVector(
            await timed('embedImage', () => client.embedImage(fixture(scene))),
        );

    for (const query of inputs.queries)
        result.queries[query] = encodeVector(
            await timed('embedImageQuery', () => client.embedImageQuery(query)),
        );

    for (const text of inputs.texts) {
        const vector = await timed('embedText', () => client.embedText(text));
        result.texts.push(vector && encodeVector(vector));
    }

    for (const text of inputs.privacy) {
        const entities = await timed('findPrivacyEntities', () => client.findPrivacyEntities(text));
        result.privacy.push(
            entities.map(({ entity_group, score, word }) => ({ group: entity_group, score, word })),
        );
    }

    for (const scene of inputs.scenes)
        result.captions[scene] = await timed('captionImage', () =>
            client.captionImage(fixture(scene)),
        );

    for (const file of Object.keys(inputs.ocr))
        result.ocr[file] = await timed('recognizeText', () => client.recognizeText(fixture(file)));

    clearInterval(sampler);
    await client.terminate();

    return { ...result, stats: { peakRss, timings } };
};

if (process.argv[2]) {
    const inputs = JSON.parse(await readFile(join(fixtures, 'inputs.json'), 'utf8'));
    const result = await runPreset(process.argv[2], inputs);
    process.send(result, () => process.exit(0));
} else {
    const inputs = await render();
    const results = {};

    for (const name of Object.keys(PRESETS)) {
        results[name] = await new Promise((resolve, reject) => {
            const child = fork(import.meta.filename, [name]);
            child.on('message', resolve);
            child.on('exit', (code) => code && reject(new Error(`${name} exited with ${code}`)));
        });
    }

    const reference = {
        images: results.low.images,
        queries: { q8: results.low.queries, fp32: results.medium.queries },
        texts: results.low.texts,
        privacy: results.low.privacy,
        captions: Object.fromEntries(
            Object.keys(PRESETS).map((name) => [name, results[name].captions]),
        ),
        ocr: { tiny: results.low.ocr, small: results.medium.ocr },
        stats: Object.fromEntries(Object.keys(PRESETS).map((name) => [name, results[name].stats])),
    };

    await writeFile(
        join(import.meta.dirname, 'reference.json'),
        JSON.stringify(reference, null, 4),
    );
}
