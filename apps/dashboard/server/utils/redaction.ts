import { isIP } from 'node:net';

export type RedactionKind = 'secret' | 'email' | 'phone' | 'card' | 'account' | 'document' | 'ip';

export type Redaction = {
    x: number;
    y: number;
    width: number;
    height: number;
    kind: RedactionKind;
};

type Size = { width: number; height: number };
type Point = [number, number];
type Span = { start: number; end: number; kind: RedactionKind };

const KIND_PRIORITY: RedactionKind[] = [
    'secret',
    'card',
    'account',
    'document',
    'email',
    'phone',
    'ip',
];

const ENTITY_KINDS: Record<string, RedactionKind> = {
    private_email: 'email',
    private_phone: 'phone',
    account_number: 'account',
    secret: 'secret',
};

const SECRET_PATTERNS = [
    /\b(?:AKIA|ASIA|ABIA|ACCA)[A-Z0-9]{16}\b/g,
    /\bgh[pousr]_[A-Za-z0-9]{30,255}/g,
    /\bgithub_pat_[A-Za-z0-9_]{22,255}/g,
    /\bglpat-[A-Za-z0-9_-]{20,}/g,
    /\bxox[abposr]-[A-Za-z0-9-]{10,}/g,
    /\b(?:sk|rk)_(?:live|test)_[A-Za-z0-9]{10,}/g,
    /\bwhsec_[A-Za-z0-9]{20,}/g,
    /\bsk-[A-Za-z0-9_-]{20,}/g,
    /\bAIza[0-9A-Za-z_-]{35}/g,
    /\bnpm_[A-Za-z0-9]{36}/g,
    /\beyJ[A-Za-z0-9_-]{8,}\.eyJ[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,}/g,
    /\b[MNO][A-Za-z\d_-]{23,25}\.[A-Za-z\d_-]{6}\.[A-Za-z\d_-]{27,}/g,
    /hooks\.slack\.com\/services\/(?<value>[A-Za-z0-9/]{20,})/g,
    /discord(?:app)?\.com\/api\/webhooks\/\d+\/(?<value>[\w-]{20,})/g,
    /\bBearer\s+(?<value>[A-Za-z0-9._~+/-]{16,}=*)/gi,
    /\b[a-z][a-z0-9+.-]*:\/\/[^\s:@/]+:(?<value>[^\s@/]+)@/gi,
    /\b[\w.-]*?(?:password|passwd|pwd|secret|token|api[_-]?key|access[_-]?key|private[_-]?key|client[_-]?secret|auth)\b["']?\s*[:=]\s*["']?(?<value>[^\s"',;]{4,})/gi,
];

const MIN_ENTITY_LENGTH = 6;

const PLACEHOLDER_VALUE =
    /^(?:true|false|null|none|undefined|\*+|x+|•+|\.+)$|^(?:\$|process\.env|os\.environ|env\()/i;

const ASSIGNMENT_PREFIX = /^[\w.-]+["']?\s*[:=]\s*["']?/;
const ASSIGNMENT_OPERATOR = /^["']?\s*[:=]/;

const PRIVATE_KEY_START = /-----BEGIN [A-Z ]*PRIVATE KEY-----/;
const PRIVATE_KEY_END = /-----END [A-Z ]*PRIVATE KEY-----/;

const EMAIL_PATTERN = /[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}/g;
const CARD_CANDIDATE = /\d(?:[\s-]?\d){11,18}/g;
const IPV4_CANDIDATE = /(?<![\w.])(?:\d{1,3}\.){3}\d{1,3}(?![\w]|\.\d)/g;
const IPV6_CANDIDATE = /(?<![\w:])[0-9A-Fa-f]{0,4}(?::[0-9A-Fa-f]{0,4}){2,7}(?![\w:])/g;
const MRZ_LINE = /^[A-Z0-9<]{28,}$/;

const isPublicIpv4 = (address: string) => {
    const [a, b, c] = address.split('.').map(Number) as [number, number, number];

    return !(
        a === 0 ||
        a === 10 ||
        a === 127 ||
        a >= 224 ||
        (a === 100 && b >= 64 && b <= 127) ||
        (a === 169 && b === 254) ||
        (a === 172 && b >= 16 && b <= 31) ||
        (a === 192 && b === 168) ||
        (a === 192 && b === 0 && (c === 0 || c === 2)) ||
        (a === 198 && (b === 18 || b === 19)) ||
        (a === 198 && b === 51 && c === 100) ||
        (a === 203 && b === 0 && c === 113)
    );
};

const isPublicIpv6 = (address: string) => {
    const lower = address.toLowerCase();
    if (lower === '::' || lower === '::1') return false;

    const first = parseInt(lower.startsWith('::') ? '0' : lower.split(':')[0]!, 16);

    return !(
        (first & 0xffc0) === 0xfe80 ||
        (first & 0xfe00) === 0xfc00 ||
        (first & 0xff00) === 0xff00 ||
        lower.startsWith('2001:db8:') ||
        first === 0
    );
};

const matchSpans = (text: string, pattern: RegExp, kind: RedactionKind) =>
    [...text.matchAll(pattern)].map((match): Span => {
        const value = match.groups?.value;
        const start = value ? match.index + match[0].lastIndexOf(value) : match.index;

        return { start, end: start + (value ?? match[0]).length, kind };
    });

const secretSpans = (text: string) =>
    SECRET_PATTERNS.flatMap((pattern) =>
        matchSpans(text, pattern, 'secret').filter(
            (span) => !PLACEHOLDER_VALUE.test(text.slice(span.start, span.end)),
        ),
    );

const numberSpans = (text: string) => {
    const upper = text.toUpperCase();
    const spans = matchSpans(upper, IBAN_PATTERN, 'account').filter((span) =>
        isValidIban(compactPii(upper.slice(span.start, span.end))),
    );

    for (const span of matchSpans(upper, CARD_CANDIDATE, 'card')) {
        const digits = compactPii(upper.slice(span.start, span.end));
        if (CARD_PATTERN.test(digits) && isValidLuhn(digits)) spans.push(span);
    }

    return spans;
};

const ipSpans = (text: string) => [
    ...matchSpans(text, IPV4_CANDIDATE, 'ip').filter((span) => {
        const address = text.slice(span.start, span.end);
        return isIP(address) === 4 && isPublicIpv4(address);
    }),
    ...matchSpans(text, IPV6_CANDIDATE, 'ip').filter((span) => {
        const address = text.slice(span.start, span.end);
        return isIP(address) === 6 && isPublicIpv6(address);
    }),
];

const escapeRegExp = (value: string) => value.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');

const entityPatterns = (entities: PrivacyEntity[]) =>
    entities.flatMap((entity) => {
        const kind = ENTITY_KINDS[entity.group];
        const word = entity.word.trim();
        if (!kind || entity.score < MIN_ENTITY_SCORE || compactPii(word).length < MIN_ENTITY_LENGTH)
            return [];

        const loose = [...word.replace(/\s+/g, '')].map(escapeRegExp).join('\\s*');
        return [{ kind, pattern: new RegExp(loose, 'g') }];
    });

const entitySpans = (text: string, patterns: ReturnType<typeof entityPatterns>) =>
    patterns.flatMap(({ kind, pattern }) =>
        matchSpans(text, pattern, kind).flatMap((span) => {
            if (kind !== 'secret') return [span];
            if (ASSIGNMENT_OPERATOR.test(text.slice(span.end))) return [];

            const start =
                span.start +
                (ASSIGNMENT_PREFIX.exec(text.slice(span.start, span.end))?.[0].length ?? 0);
            return start < span.end ? [{ ...span, start }] : [];
        }),
    );

const mergeSpans = (spans: Span[]) => {
    const merged: Span[] = [];

    for (const span of spans.toSorted((a, b) => a.start - b.start)) {
        const last = merged.at(-1);

        if (last && span.start <= last.end + 1) {
            last.end = Math.max(last.end, span.end);
            if (KIND_PRIORITY.indexOf(span.kind) < KIND_PRIORITY.indexOf(last.kind))
                last.kind = span.kind;
        } else {
            merged.push({ ...span });
        }
    }

    return merged;
};

const codePointOffsets = (text: string) => {
    const offsets = new Array<number>(text.length + 1);
    let count = 0;

    for (let index = 0; index <= text.length; index++) {
        offsets[index] = count;
        const code = text.charCodeAt(index);
        if (!(code >= 0xd800 && code <= 0xdbff && index + 1 < text.length)) count++;
    }

    return offsets;
};

const along = (from: Point, to: Point, t: number): Point => [
    from[0] + (to[0] - from[0]) * t,
    from[1] + (to[1] - from[1]) * t,
];

const spanRect = (line: OcrLine, span: Span, scale: Point, size: Size): Redaction => {
    const [startTop, endTop, endBottom, startBottom] = line.box;
    const offsets = codePointOffsets(line.text);
    const length = offsets[line.text.length]!;
    const padding = 0.35 / Math.max(1, length);
    const from = Math.max(0, (line.characters?.[offsets[span.start]!] ?? 0) - padding);
    const to = Math.min(1, (line.characters?.[offsets[span.end]!] ?? 1) + padding);

    const corners = [
        along(startTop, endTop, from),
        along(startTop, endTop, to),
        along(startBottom, endBottom, to),
        along(startBottom, endBottom, from),
    ];
    const lineHeight = Math.hypot(startBottom[0] - startTop[0], startBottom[1] - startTop[1]);
    const margin = lineHeight * 0.15;
    const xs = corners.map(([x]) => x);
    const ys = corners.map(([, y]) => y);

    const left = Math.max(0, Math.floor((Math.min(...xs) - margin) * scale[0]));
    const top = Math.max(0, Math.floor((Math.min(...ys) - margin) * scale[1]));
    const right = Math.min(size.width, Math.ceil((Math.max(...xs) + margin) * scale[0]));
    const bottom = Math.min(size.height, Math.ceil((Math.max(...ys) + margin) * scale[1]));

    return { x: left, y: top, width: right - left, height: bottom - top, kind: span.kind };
};

export const findRedactions = (
    lines: OcrLine[],
    entities: PrivacyEntity[],
    scanned: Size,
    original: Size,
) => {
    const patterns = entityPatterns(entities);
    const hasMrz = hasValidMrz(lines.map((line) => line.text).join('\n'));
    const scale: Point = [original.width / scanned.width, original.height / scanned.height];
    const redactions: Redaction[] = [];
    let isInPrivateKey = false;

    for (const line of lines) {
        const { text } = line;
        const isKeyStart = PRIVATE_KEY_START.test(text);
        const isKeyLine = isInPrivateKey || isKeyStart;
        if (isKeyStart) isInPrivateKey = true;
        if (PRIVATE_KEY_END.test(text)) isInPrivateKey = false;

        const spans: Span[] =
            isKeyLine || (hasMrz && MRZ_LINE.test(text.replace(/\s+/g, '')))
                ? [{ start: 0, end: text.length, kind: isKeyLine ? 'secret' : 'document' }]
                : [
                      ...secretSpans(text),
                      ...numberSpans(text),
                      ...matchSpans(text, EMAIL_PATTERN, 'email'),
                      ...ipSpans(text),
                      ...entitySpans(text, patterns),
                  ];

        for (const span of mergeSpans(spans)) {
            const rect = spanRect(line, span, scale, original);
            if (rect.width > 0 && rect.height > 0) redactions.push(rect);
        }
    }

    return redactions;
};
