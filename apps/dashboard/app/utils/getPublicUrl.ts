export const getPublicUrl = (route: `/${string}`) => {
    const domains = useDomains();
    const reqUrl = useRequestURL({ xForwardedHost: true, xForwardedProto: true });

    const { returnHttps } = usePublicSettings().value;

    const protocol =
        returnHttps === 'auto'
            ? reqUrl.protocol.slice(0, -1)
            : returnHttps === 'true'
              ? 'https'
              : 'http';

    const domain = domains.value.length
        ? domains.value[Math.floor(Math.random() * domains.value.length)]
        : reqUrl.host;

    return `${protocol}://${domain}${route}`;
};
