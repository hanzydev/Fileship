import semver from 'semver';

import pkg from '../../../../../package.json';

const repoUrl = 'hanzydev/Fileship';

type Channel = InstanceSettings['updateChannel'];
type Release = { tag_name: string; draft: boolean; prerelease: boolean };

const latestRelease = async (channel: Channel) => {
    const releases = await $fetch<Release[]>(`https://api.github.com/repos/${repoUrl}/releases`, {
        query: { per_page: 30 },
        headers: { 'User-Agent': 'Fileship-App' },
        timeout: 5_000,
    });

    return releases
        .filter((release) => !release.draft && (channel === 'experimental' || !release.prerelease))
        .map((release) => semver.clean(release.tag_name))
        .filter((version) => version !== null)
        .sort(semver.rcompare)[0];
};

const cachedCheckVersion = defineCachedFunction(
    async (channel: Channel) => {
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
            const latestVersion = (await latestRelease(channel)) ?? pkg.version;

            return {
                hasUpdate: semver.gt(latestVersion, pkg.version),
                latestVersion,
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
        getKey: (channel: Channel) => `ua-status-${pkg.version}-${channel}`,
    },
);

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const versionInfo = await cachedCheckVersion((await getSettings()).updateChannel);

    const targetTag = versionInfo.hasUpdate ? versionInfo.latestVersion : pkg.version;

    return {
        version: pkg.version,
        url: `https://github.com/${repoUrl}/releases/tag/v${targetTag}`,
        hasUpdate: versionInfo.hasUpdate,
        latestVersion: versionInfo.latestVersion,
        updaterAvailable: versionInfo.updaterAvailable,
    };
});
