import { isIP } from 'node:net';

import type { H3Event } from 'h3';

const isPrivateAddress = (address: string) => {
    const value = address.replace(/^::ffff:/i, '');

    if (isIP(value) === 4) {
        const [a = 0, b = 0] = value.split('.').map(Number);
        return (
            a === 10 || a === 127 || (a === 172 && b >= 16 && b <= 31) || (a === 192 && b === 168)
        );
    }

    const lower = value.toLowerCase();
    return lower === '::1' || lower.startsWith('fc') || lower.startsWith('fd');
};

export const getClientIp = (event: H3Event) => {
    const peer = getRequestIP(event) ?? '';

    if (!peer || isPrivateAddress(peer)) {
        const proxied = getRequestHeader(event, 'x-fileship-client-ip')?.trim();
        if (proxied && isIP(proxied)) return proxied;

        const forwarded = getRequestHeader(event, 'x-forwarded-for')
            ?.split(',')
            .map((entry) => entry.trim())
            .filter((entry) => isIP(entry));

        if (forwarded?.length) return forwarded.at(-1)!;
    }

    return peer || 'Unknown';
};
