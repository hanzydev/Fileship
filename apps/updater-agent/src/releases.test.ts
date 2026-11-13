import semver from 'semver';
import { describe, expect, it } from 'vitest';

import { pickRelease, type Release } from './releases';

const release = (tag: string, prerelease = false, draft = false): Release => ({
    tag_name: tag,
    prerelease,
    draft,
});

const releases = [
    release('v6.9.0-beta.2', true),
    release('v6.9.0-beta.1', true),
    release('v6.8.1'),
    release('v6.8.0'),
    release('v7.0.0', false, true),
];

describe('pickRelease', () => {
    it('skips pre-releases and drafts on the stable channel', () => {
        expect(pickRelease(releases, 'stable')).toBe('6.8.1');
    });

    it('takes the newest release on the experimental channel', () => {
        expect(pickRelease(releases, 'experimental')).toBe('6.9.0-beta.2');
    });

    it('gives experimental users a stable release that is newer than every pre-release', () => {
        expect(pickRelease([release('v6.9.0'), ...releases], 'experimental')).toBe('6.9.0');
    });

    it('never offers a lower version after switching back to stable', () => {
        expect(semver.gt(pickRelease(releases, 'stable')!, '6.9.0-beta.2')).toBe(false);
        expect(
            semver.gt(pickRelease([release('v6.9.0'), ...releases], 'stable')!, '6.9.0-beta.2'),
        ).toBe(true);
    });

    it('returns nothing without releases', () => {
        expect(pickRelease([], 'stable')).toBeUndefined();
    });
});
