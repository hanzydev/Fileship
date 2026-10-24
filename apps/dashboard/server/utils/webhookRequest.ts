import { lookup, promises as dns } from 'node:dns';
import { request as httpRequest } from 'node:http';
import { request as httpsRequest } from 'node:https';
import { BlockList, isIP, type LookupFunction } from 'node:net';

export type WebhookFailure = 'blocked' | 'redirects' | 'invalidUrl' | 'timeout' | 'network';

export class WebhookRequestError extends Error {
    constructor(
        public reason: WebhookFailure,
        public detail?: string,
    ) {
        super(detail ?? reason);
    }
}

export type WebhookResponse = {
    status: number;
    retryAfter: number | null;
    body: string;
};

const REDIRECTS = [301, 302, 303, 307, 308];
const MAX_REDIRECTS = 3;
const MAX_BODY = 1024;

const IPV4_RANGES: [string, number][] = [
    ['0.0.0.0', 8],
    ['10.0.0.0', 8],
    ['100.64.0.0', 10],
    ['127.0.0.0', 8],
    ['169.254.0.0', 16],
    ['172.16.0.0', 12],
    ['192.0.0.0', 24],
    ['192.0.2.0', 24],
    ['192.88.99.0', 24],
    ['192.168.0.0', 16],
    ['198.18.0.0', 15],
    ['198.51.100.0', 24],
    ['203.0.113.0', 24],
    ['224.0.0.0', 4],
    ['240.0.0.0', 4],
];

const IPV6_RANGES: [string, number][] = [
    ['::', 96],
    ['64:ff9b:1::', 48],
    ['100::', 64],
    ['2001:db8::', 32],
    ['fc00::', 7],
    ['fe80::', 10],
    ['fec0::', 10],
    ['ff00::', 8],
];

const blockList = new BlockList();

for (const [network, prefix] of IPV4_RANGES) {
    const [a, b, c, d] = network.split('.').map(Number) as [number, number, number, number];
    const embedded = `${((a << 8) | b).toString(16)}:${((c << 8) | d).toString(16)}`;

    blockList.addSubnet(network, prefix, 'ipv4');
    blockList.addSubnet(`64:ff9b::${embedded}`, 96 + prefix, 'ipv6');
    blockList.addSubnet(`2002:${embedded}::`, 16 + prefix, 'ipv6');
}

for (const [network, prefix] of IPV6_RANGES) blockList.addSubnet(network, prefix, 'ipv6');

const isBlockedAddress = (address: string) =>
    blockList.check(address, isIP(address) === 6 ? 'ipv6' : 'ipv4');

const hostOf = (url: URL) => url.hostname.replace(/^\[(.*)\]$/, '$1');

const guardedLookup =
    (allowPrivate: boolean): LookupFunction =>
    (hostname, options, callback) =>
        lookup(hostname, { ...options, all: true }, (error, addresses) => {
            if (error) return callback(error, '', 0);

            if (!allowPrivate && addresses.some((entry) => isBlockedAddress(entry.address)))
                return callback(new WebhookRequestError('blocked', hostname), '', 0);

            if (options.all) return (callback as any)(null, addresses);

            callback(null, addresses[0]!.address, addresses[0]!.family);
        });

export const assertWebhookTarget = async (value: string, allowPrivate: boolean) => {
    const url = new URL(value);

    if (!['http:', 'https:'].includes(url.protocol)) throw new WebhookRequestError('invalidUrl');
    if (allowPrivate) return;

    const host = hostOf(url);
    const addresses = isIP(host)
        ? [host]
        : await dns
              .lookup(host, { all: true })
              .then((entries) => entries.map((entry) => entry.address))
              .catch(() => []);

    if (addresses.some(isBlockedAddress)) throw new WebhookRequestError('blocked', host);
};

const parseRetryAfter = (value: string | string[] | undefined) => {
    const seconds = Number(Array.isArray(value) ? value[0] : value);
    return Number.isFinite(seconds) && seconds >= 0 ? seconds : null;
};

const post = (
    url: URL,
    body: string,
    headers: Record<string, string>,
    allowPrivate: boolean,
    signal: AbortSignal,
) =>
    new Promise<WebhookResponse & { location?: string }>((resolve, reject) => {
        const host = hostOf(url);

        if (!['http:', 'https:'].includes(url.protocol))
            return reject(new WebhookRequestError('invalidUrl'));

        if (!allowPrivate && isIP(host) && isBlockedAddress(host))
            return reject(new WebhookRequestError('blocked', host));

        const request = (url.protocol === 'https:' ? httpsRequest : httpRequest)(
            url,
            {
                method: 'POST',
                headers: { ...headers, 'Content-Length': Buffer.byteLength(body).toString() },
                lookup: guardedLookup(allowPrivate),
                signal,
            },
            (response) => {
                const chunks: Buffer[] = [];
                let size = 0;

                const finish = () => {
                    response.destroy();
                    resolve({
                        status: response.statusCode!,
                        retryAfter: parseRetryAfter(response.headers['retry-after']),
                        body: Buffer.concat(chunks).toString('utf8').slice(0, MAX_BODY),
                        location: response.headers.location,
                    });
                };

                response.on('data', (chunk: Buffer) => {
                    chunks.push(chunk);
                    size += chunk.length;
                    if (size >= MAX_BODY) finish();
                });
                response.on('end', finish);
                response.on('error', finish);
            },
        );

        request.on('error', (error: any) => {
            if (error instanceof WebhookRequestError) return reject(error);
            if (error?.name === 'AbortError') return reject(new WebhookRequestError('timeout'));
            reject(new WebhookRequestError('network', error?.code ?? error?.message));
        });

        request.end(body);
    });

export const sendWebhookRequest = async (
    target: string,
    body: string,
    headers: Record<string, string>,
    options: { allowPrivate: boolean; timeout: number },
) => {
    const signal = AbortSignal.timeout(options.timeout);
    let url = new URL(target);

    for (let hop = 0; hop <= MAX_REDIRECTS; hop++) {
        const { location, ...response } = await post(
            url,
            body,
            headers,
            options.allowPrivate,
            signal,
        );

        if (!REDIRECTS.includes(response.status) || !location) return response;

        url = new URL(location, url);
    }

    throw new WebhookRequestError('redirects');
};
