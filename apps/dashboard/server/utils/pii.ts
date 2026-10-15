import type { PrivacyEntity } from '@fileship/ai';

const MIN_ENTITY_SCORE = 0.5;

const ENTITY_REASONS: Record<string, string> = {
    private_email: 'EMAIL',
    private_phone: 'PHONE',
    private_address: 'ADDRESS',
    account_number: 'ACCOUNT_NUMBER',
    secret: 'SECRET',
    private_person: 'PERSON_NAME',
    private_date: 'PRIVATE_DATE',
};

const SUPPORTING_REASONS = ['PERSON_NAME', 'PRIVATE_DATE'];

const CARD_PATTERN =
    /^(?:4\d{12}(?:\d{3}){0,2}|5[1-5]\d{14}|2(?:2[2-9]|[3-6]\d|7[01]|720)\d{12,13}|3[47]\d{13}|3(?:0[0-5]|[689]\d)\d{11,16}|35\d{14,17}|6\d{15,18}|220[0-4]\d{12}|9792\d{12})$/;

const CARD_PRINT = /\b(?:valid|good)\s?(?:thru|until)\b|\bexp(?:iry|ires)?\b/i;

const CAPTION_DOCUMENTS: [RegExp, string][] = [
    [/\b(?:credit|debit|bank|payment)\s+cards?\b/i, 'PAYMENT_CARD'],
    [
        /\b(?:passports?|id cards?|identity cards?|identification cards?|driver'?s licen[cs]es?)\b/i,
        'ID_DOCUMENT',
    ],
];

const IBAN_PATTERN = /\b[A-Z]{2}\d{2}(?:\s?[A-Z0-9]{4}){2,7}(?:\s?[A-Z0-9]{1,4})?\b/g;

const compact = (value: string) => value.toUpperCase().replace(/[^A-Z0-9]/g, '');

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

const mrzCheckDigit = (value: string) =>
    [...value].reduce((sum, char, index) => {
        const code = char === '<' ? 0 : /\d/.test(char) ? +char : char.charCodeAt(0) - 55;
        return sum + code * [7, 3, 1][index % 3]!;
    }, 0) % 10;

const hasValidMrz = (text: string) => {
    if (!text.includes('<<')) return false;

    const compacted = text.toUpperCase().replace(/\s+/g, '');

    const passport = compacted.match(/([A-Z0-9<]{9})(\d)[A-Z<]{3}(\d{6})(\d)[MFX<](\d{6})(\d)/g);
    const idCard = compacted.match(/[ACI][A-Z<][A-Z<]{3}([A-Z0-9<]{9})(\d)/g);

    return (
        passport?.some((line) => {
            const [, number, numberCheck, birth, birthCheck, expiry, expiryCheck] = line.match(
                /([A-Z0-9<]{9})(\d)[A-Z<]{3}(\d{6})(\d)[MFX<](\d{6})(\d)/,
            )!;

            return (
                mrzCheckDigit(number!) === +numberCheck! &&
                mrzCheckDigit(birth!) === +birthCheck! &&
                mrzCheckDigit(expiry!) === +expiryCheck!
            );
        }) ||
        idCard?.some((line) => {
            const [, number, check] = line.match(/[ACI][A-Z<][A-Z<]{3}([A-Z0-9<]{9})(\d)/)!;
            return /\d/.test(number!) && mrzCheckDigit(number!) === +check!;
        }) ||
        false
    );
};

const findValidatedPii = (text: string) => {
    const reasons = new Set<string>();
    const numbers: string[] = [];
    const upper = text.toUpperCase();

    for (const match of upper.match(IBAN_PATTERN) ?? []) {
        const iban = compact(match);

        if (isValidIban(iban)) {
            reasons.add('IBAN');
            numbers.push(iban);
        }
    }

    for (const match of upper.replace(IBAN_PATTERN, ' ').match(/\d(?:[\s-]?\d){11,18}/g) ?? []) {
        const digits = compact(match);

        if (CARD_PATTERN.test(digits) && isValidLuhn(digits)) {
            reasons.add('PAYMENT_CARD');
            numbers.push(digits);
        } else if (CARD_PRINT.test(text) && /\b(?:0[1-9]|1[0-2])\s?\/\s?\d{2}\b/.test(text)) {
            reasons.add('PAYMENT_CARD');
            numbers.push(digits);
        }
    }

    if (hasValidMrz(text)) reasons.add('ID_DOCUMENT');

    return { reasons, numbers };
};

export const findPii = (text: string, entities: PrivacyEntity[], caption = '') => {
    const { reasons, numbers } = findValidatedPii(text);

    if (/\d(?:[\s-]?\d){5,}/.test(text))
        for (const [pattern, reason] of CAPTION_DOCUMENTS)
            if (pattern.test(caption)) reasons.add(reason);

    for (const entity of entities) {
        const reason = ENTITY_REASONS[entity.entity_group];
        if (!reason || entity.score < MIN_ENTITY_SCORE) continue;

        const value = compact(entity.word);

        if (
            value.length < 3 ||
            (value.length >= 4 &&
                numbers.some((number) => number.includes(value) || value.includes(number))) ||
            (reasons.has('ID_DOCUMENT') && entity.word.includes('<'))
        )
            continue;

        reasons.add(reason);
    }

    const isSensitive =
        [...reasons].some((reason) => !SUPPORTING_REASONS.includes(reason)) ||
        SUPPORTING_REASONS.every((reason) => reasons.has(reason));

    return isSensitive ? reasons : new Set<string>();
};
