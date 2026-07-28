import pkg from '../../../../../package.json';

const repoUrl = 'hanzydev/Fileship';

const cachedCheckVersion = defineCachedFunction(
    async () => {
        try {
            const updaterStatus = await updaterFetch<{
                hasUpdate: boolean;
                latestVersion?: string;
            }>(`/api/status`);

            if (updaterStatus) {
                return {
                    hasUpdate: updaterStatus.hasUpdate,
                    latestVersion: updaterStatus.latestVersion || pkg.version,
                    updaterAvailable: true,
                };
            }
        } catch {
            //
        }

        try {
            const release = await $fetch<{ tag_name: string }>(
                `https://api.github.com/repos/${repoUrl}/releases/latest`,
                {
                    headers: { 'User-Agent': 'Fileship-App' },
                    timeout: 5_000,
                },
            );

            const cleanTag = release.tag_name.replace(/^v/, '');

            return {
                hasUpdate: pkg.version !== cleanTag,
                latestVersion: cleanTag,
                updaterAvailable: false,
            };
        } catch {
            return {
                hasUpdate: false,
                latestVersion: pkg.version,
                updaterAvailable: false,
            };
        }
    },
    {
        maxAge: 1_800,
        getKey: () => `ua-status-${pkg.version}`,
    },
);

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const versionInfo = await cachedCheckVersion();

    const targetTag = versionInfo.hasUpdate ? versionInfo.latestVersion : pkg.version;

    return {
        version: pkg.version,
        url: `https://github.com/${repoUrl}/releases/tag/v${targetTag}`,
        hasUpdate: versionInfo.hasUpdate,
        latestVersion: versionInfo.latestVersion,
        updaterAvailable: versionInfo.updaterAvailable,
    };
});
