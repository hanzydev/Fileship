import type { H3Event } from 'h3';

export const buildPublicUrl = (event: H3Event, domains: string[], route: `/${string}`) => {
    const reqUrl = getRequestURL(event, { xForwardedHost: true, xForwardedProto: true });
    const { returnHttps } = getSettingsSync();

    const protocol =
        returnHttps === 'auto'
            ? reqUrl.protocol.slice(0, -1)
            : returnHttps === 'true'
              ? 'https'
              : 'http';

    const domain = domains.length
        ? domains[Math.floor(Math.random() * domains.length)]
        : reqUrl.host;

    return `${protocol}://${domain}${route}`;
};
