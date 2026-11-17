import { consola } from 'consola';
import semver from 'semver';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';

import { cacheReleases, pickRelease, type Release } from './releases';

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

describe('cacheReleases', () => {
    beforeEach(() => {
        vi.useFakeTimers();
        vi.spyOn(consola, 'warn').mockImplementation(() => undefined);
    });

    afterEach(() => {
        vi.useRealTimers();
        vi.restoreAllMocks();
    });

    it('asks GitHub once while the list is fresh', async () => {
        const fetchReleases = vi.fn().mockResolvedValue(releases);
        const getReleases = cacheReleases(fetchReleases, 60_000);

        expect(await getReleases()).toBe(releases);
        vi.advanceTimersByTime(59_000);
        expect(await getReleases()).toBe(releases);
        expect(fetchReleases).toHaveBeenCalledTimes(1);
    });

    it('asks again once the list is older than its maximum age', async () => {
        const newer = [release('v6.9.0-beta.3', true), ...releases];
        const fetchReleases = vi.fn().mockResolvedValueOnce(releases).mockResolvedValue(newer);
        const getReleases = cacheReleases(fetchReleases, 60_000);

        await getReleases();
        vi.advanceTimersByTime(60_000);

        expect(await getReleases()).toBe(newer);
        expect(fetchReleases).toHaveBeenCalledTimes(2);
    });

    it('keeps the last list when GitHub refuses', async () => {
        const fetchReleases = vi
            .fn()
            .mockResolvedValueOnce(releases)
            .mockRejectedValue(new Error('rate limit exceeded'));
        const getReleases = cacheReleases(fetchReleases, 60_000);

        await getReleases();
        vi.advanceTimersByTime(60_000);

        expect(await getReleases()).toBe(releases);
        expect(fetchReleases).toHaveBeenCalledTimes(2);
    });

    it('fails when GitHub refuses before any list was fetched', async () => {
        const getReleases = cacheReleases(
            vi.fn().mockRejectedValue(new Error('rate limit exceeded')),
            60_000,
        );

        await expect(getReleases()).rejects.toThrow('rate limit exceeded');
    });
});
