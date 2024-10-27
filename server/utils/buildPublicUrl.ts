import type { H3Event } from 'h3';

export const buildPublicUrl = (
    event: H3Event,
    domains: string[],
    route: `/${string}`,
) => {
    const reqUrl = getRequestURL(event);

    const protocol = process.env.NUXT_PUBLIC_RETURN_HTTPS
        ? process.env.NUXT_PUBLIC_RETURN_HTTPS === 'true'
            ? 'https'
            : 'http'
        : reqUrl.protocol.slice(0, -1);

    const domain = domains.length
        ? domains[Math.floor(Math.random() * domains.length)]
        : reqUrl.host;

    return `${protocol}://${domain}${route}`;
};
