export const buildPublicUrl = (route: `/${string}`) => {
    const runtimeConfig = useRuntimeConfig();
    const domains = useDomains();
    const reqUrl = useRequestURL({ xForwardedHost: true, xForwardedProto: true });

    const returnHttps = runtimeConfig.public.returnHttps;

    const protocol =
        returnHttps === 'auto' ? reqUrl.protocol.slice(0, -1) : returnHttps ? 'https' : 'http';

    const domain = domains.value.length
        ? domains.value[Math.floor(Math.random() * domains.value.length)]
        : reqUrl.host;

    return `${protocol}://${domain}${route}`;
};
