import semver from 'semver';

export type Channel = 'stable' | 'experimental';

export type Release = { tag_name: string; draft: boolean; prerelease: boolean };

export const pickRelease = (releases: Release[], channel: Channel) =>
    releases
        .filter((release) => !release.draft && (channel === 'experimental' || !release.prerelease))
        .map((release) => semver.clean(release.tag_name))
        .filter((version) => version !== null)
        .sort(semver.rcompare)[0];
