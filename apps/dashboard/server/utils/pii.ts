const CARD_EVIDENCE =
    /valid\s?(?:thru|until|from)|good\s?thru|card\s?(?:number|holder)|exp(?:iry|iration)\s?date|\bcvv\b|\bcvc\b|son\s?kullanma|kart\s?(?:numarası|sahibi)/i;

const ID_EVIDENCE =
    /kimlik|n[üu]fus|identity\s?(?:card|document)|\bid\s?card|passport|pasaport|driv(?:er'?s?|ing)\s?licen[cs]e|ehliyet|s[üu]r[üu]c[üu]\s?belgesi|personalausweis|carte\s?d'identit|date\s?of\s?birth|do[ğg]um\s?tarihi/i;

const VISUAL_REASONS: Record<string, string> = {
    'credit card': 'PAYMENT_CARD',
    'debit card': 'PAYMENT_CARD',
    'identity card': 'ID_DOCUMENT',
    'id card': 'ID_DOCUMENT',
    passport: 'ID_DOCUMENT',
    "driver's license": 'ID_DOCUMENT',
};

const IBAN_PATTERN = /\b[A-Z]{2}\d{2}(?:\s?[A-Z0-9]{4}){2,7}(?:\s?[A-Z0-9]{1,4})?\b/g;

const digitsOf = (value: string) => value.replace(/\D/g, '');

const isValidLuhn = (digits: string) => {
    let sum = 0;

    for (let index = 0; index < digits.length; index++) {
        let digit = +digits[digits.length - 1 - index]!;
        if (index % 2 && (digit *= 2) > 9) digit -= 9;
        sum += digit;
    }

    return sum % 10 === 0;
};

const isValidIban = (iban: string) => {
    const rearranged = `${iban.slice(4)}${iban.slice(0, 4)}`.replace(/[A-Z]/g, (letter) =>
        String(letter.charCodeAt(0) - 55),
    );

    return BigInt(rearranged) % 97n === 1n;
};

const isValidTcKimlik = (digits: string) => {
    const d = [...digits].map(Number);
    const odd = d[0]! + d[2]! + d[4]! + d[6]! + d[8]!;
    const even = d[1]! + d[3]! + d[5]! + d[7]!;

    return (
        (((odd * 7 - even) % 10) + 10) % 10 === d[9] &&
        d.slice(0, 10).reduce((total, digit) => total + digit, 0) % 10 === d[10]
    );
};

export const hasDocumentEvidence = (text: string) =>
    CARD_EVIDENCE.test(text) || ID_EVIDENCE.test(text);

export const findDocumentPii = (text: string) => {
    const reasons = new Set<string>();

    if (CARD_EVIDENCE.test(text) && /\d(?:[\s-]?\d){11,18}/.test(text)) reasons.add('PAYMENT_CARD');
    if (ID_EVIDENCE.test(text) && /\d{6,}/.test(text.replace(/(\d)\s(?=\d)/g, '$1')))
        reasons.add('ID_DOCUMENT');

    return reasons;
};

export const getVisualPiiReason = (label: string) => VISUAL_REASONS[label.toLowerCase().trim()];

export const findPii = (text: string) => {
    const reasons = new Set<string>();

    if (/[\w.+-]+@[\w-]+(?:\.[\w-]+)*\.[a-z]{2,}/i.test(text)) reasons.add('EMAIL');

    const ibans = text.toUpperCase().match(IBAN_PATTERN) ?? [];
    if (ibans.some((iban) => isValidIban(iban.replace(/\s/g, '')))) reasons.add('IBAN');

    for (const match of text
        .toUpperCase()
        .replace(IBAN_PATTERN, ' ')
        .match(/\+?\d[\d\s().-]{8,22}\d/g) ?? []) {
        const digits = digitsOf(match);

        if (digits.length >= 13 && digits.length <= 19 && isValidLuhn(digits))
            reasons.add('PAYMENT_CARD');
        else if (digits.length === 11 && /^[1-9]/.test(digits) && isValidTcKimlik(digits))
            reasons.add('NATIONAL_ID');
        else if (
            (match.trim().startsWith('+') && digits.length >= 10 && digits.length <= 15) ||
            /^0?5\d{9}$/.test(digits)
        )
            reasons.add('PHONE');
    }

    if (/P[<A-Z][A-Z]{3}[A-Z<]*<<[A-Z<]+|[A-Z0-9<]{9}\d[A-Z]{3}\d{6}[0-9<]/.test(text))
        reasons.add('PASSPORT');

    return reasons;
};
