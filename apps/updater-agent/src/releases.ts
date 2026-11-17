import { consola } from 'consola';
import semver from 'semver';

export type Channel = 'stable' | 'experimental';

export type Release = { tag_name: string; draft: boolean; prerelease: boolean };

export const pickRelease = (releases: Release[], channel: Channel) =>
    releases
        .filter((release) => !release.draft && (channel === 'experimental' || !release.prerelease))
        .map((release) => semver.clean(release.tag_name))
        .filter((version) => version !== null)
        .sort(semver.rcompare)[0];

export const cacheReleases = (fetchReleases: () => Promise<Release[]>, maxAge: number) => {
    let cached: { releases: Release[]; fetchedAt: number } | undefined;

    return async () => {
        if (cached && Date.now() - cached.fetchedAt < maxAge) return cached.releases;

        try {
            cached = { releases: await fetchReleases(), fetchedAt: Date.now() };
        } catch (error) {
            if (!cached) throw error;
            consola.warn('Could not fetch the releases from GitHub, using the last list:', error);
        }

        return cached.releases;
    };
};
