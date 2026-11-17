import semver from 'semver';

import pkg from '../../../../../package.json';

const repoUrl = 'hanzydev/Fileship';

type Channel = InstanceSettings['updateChannel'];
type Release = { tag_name: string; draft: boolean; prerelease: boolean };
type VersionInfo = {
    state: 'upToDate' | 'available' | 'publishing' | 'failed';
    latestVersion: string;
    updaterAvailable: boolean;
    composeOutdated: boolean;
};

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

const checkVersion = async (channel: Channel): Promise<VersionInfo> => {
    try {
        const updaterStatus = await updaterFetch<{
            hasUpdate: boolean;
            publishing?: boolean;
            keepsVersion?: boolean;
            latestVersion: string | null;
        }>(`/api/status`);

        return {
            state: updaterStatus.hasUpdate
                ? 'available'
                : updaterStatus.publishing
                  ? 'publishing'
                  : 'upToDate',
            latestVersion: updaterStatus.latestVersion || pkg.version,
            updaterAvailable: true,
            composeOutdated: updaterStatus.keepsVersion === false,
        };
    } catch {
        //
    }

    try {
        const latestVersion = (await latestRelease(channel)) ?? pkg.version;

        return {
            state: semver.gt(latestVersion, pkg.version) ? 'available' : 'upToDate',
            latestVersion,
            updaterAvailable: false,
            composeOutdated: false,
        };
    } catch {
        return {
            state: 'failed',
            latestVersion: pkg.version,
            updaterAvailable: false,
            composeOutdated: false,
        };
    }
};

const cachedCheckVersion = defineCachedFunction(
    (channel: Channel, _refresh: boolean) => checkVersion(channel),
    {
        maxAge: 300,
        swr: false,
        getKey: (channel) => `ua-status-${pkg.version}-${channel}`,
        shouldInvalidateCache: (_channel, refresh) => refresh,
        validate: (entry) =>
            !!entry.value &&
            (['upToDate', 'available'].includes(entry.value.state) ||
                Date.now() - entry.mtime! < 60_000),
    },
);

export default defineEventHandler(async (event) => {
    adminOnly(event);

    const versionInfo = await cachedCheckVersion(
        (await getSettings()).updateChannel,
        getQuery(event).refresh === 'true',
    );

    const targetTag = versionInfo.state === 'available' ? versionInfo.latestVersion : pkg.version;

    return {
        version: pkg.version,
        url: `https://github.com/${repoUrl}/releases/tag/v${targetTag}`,
        state: versionInfo.state,
        latestVersion: versionInfo.latestVersion,
        updaterAvailable: versionInfo.updaterAvailable,
        composeOutdated: versionInfo.composeOutdated,
    };
});
