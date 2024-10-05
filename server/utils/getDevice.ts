export const getDevice = async (
    headers: Record<string, string>,
    ip: string,
) => {
    const userAgent = headers['user-agent'];

    const oss = ['Android', 'iPhone', 'Windows', 'Macintosh', 'Linux'];
    const platforms = ['OPR', 'Edge', 'Chrome', 'Firefox', 'Safari'];

    let os = oss.find((o) => userAgent?.includes(o)) || 'Unknown';

    if (os === 'iPhone' || os === 'Android') os = 'Mobile';
    if (os === 'Windows' || os === 'Macintosh' || os === 'Linux')
        os = 'Desktop';

    const platform = platforms.find((p) => userAgent?.includes(p)) || 'Unknown';

    let ipInfo = {
        city: 'Unknown',
        regionName: 'Unknown',
        country: 'Unknown',
    };

    try {
        const res = await $fetch<{
            status: 'success';
            city: string;
            regionName: string;
            country: string;
        }>(`http://ip-api.com/json/${ip}`, { ignoreResponseError: true });

        if (res.status === 'success') ipInfo = res;
    } catch {
        //
    }

    return {
        os,
        platform,
        location: `${ipInfo.city}, ${ipInfo.regionName}, ${ipInfo.country}`,
    };
};
