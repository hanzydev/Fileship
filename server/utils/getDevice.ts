export const getDevice = async (
    headers: Record<string, string>,
    ip: string,
) => {
    const userAgent = headers['user-agent'];

    const oss = ['Android', 'iPhone', 'Windows', 'Macintosh', 'Linux'];
    const platforms = ['OPR', 'Edge', 'Chrome', 'Firefox', 'Safari'];

    let os = oss.find((p) => userAgent?.includes(p)) || 'Unknown';

    if (os === 'iPhone' || os === 'Android') os = 'Mobile';
    if (os === 'Windows' || os === 'Macintosh' || os === 'Linux')
        os = 'Desktop';

    const platform = platforms.find((b) => userAgent?.includes(b)) || 'Unknown';

    let ipInfo = await $fetch<{
        city: string;
        region: string;
        country: string;
    }>(`https://ipinfo.io/${ip}/json`, { ignoreResponseError: true });

    if ('error' in ipInfo || 'bogon' in ipInfo) {
        ipInfo = {
            city: 'Unknown',
            region: 'Unknown',
            country: 'Unknown',
        };
    }

    return {
        os,
        platform,
        location: `${ipInfo.city}, ${ipInfo.region}, ${ipInfo.country}`,
    };
};
