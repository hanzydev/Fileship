export const getDevice = (userAgent: string) => {
    const oss = ['Android', 'iPhone', 'Windows', 'Macintosh', 'Linux'];
    const platforms = ['OPR', 'Edge', 'Chrome', 'Firefox', 'Safari'];

    let os = oss.find((o) => userAgent.includes(o)) || 'Unknown';

    if (os === 'iPhone' || os === 'Android') os = 'Mobile';
    if (os === 'Windows' || os === 'Macintosh' || os === 'Linux') os = 'Desktop';

    const platform = platforms.find((p) => userAgent.includes(p)) || 'Unknown';

    return { os, platform };
};

export const getLocation = async (ip: string) => {
    const res = await $fetch<{
        status: 'success' | 'fail';
        city: string;
        regionName: string;
        country: string;
    }>(`http://ip-api.com/json/${ip}`, { ignoreResponseError: true }).catch(() => null);

    return res?.status === 'success'
        ? `${res.city}, ${res.regionName}, ${res.country}`
        : 'Unknown, Unknown, Unknown';
};
