import { describe, expect, it } from 'vitest';

import { setEnvValue, versionToPin } from './versionPin';

describe('versionToPin', () => {
    it('pins every version on the experimental channel', () => {
        expect(versionToPin('experimental', '6.9.0-beta.4')).toBe('6.9.0-beta.4');
        expect(versionToPin('experimental', '6.9.0')).toBe('6.9.0');
    });

    it('keeps a pre-release pinned after switching back to stable', () => {
        expect(versionToPin('stable', '6.9.0-beta.4')).toBe('6.9.0-beta.4');
    });

    it('follows the latest images for stable releases on the stable channel', () => {
        expect(versionToPin('stable', '6.8.1')).toBeUndefined();
    });
});

describe('setEnvValue', () => {
    it('adds the line to an empty or missing file', () => {
        expect(setEnvValue('', 'FILESHIP_VERSION', '6.9.0-beta.4')).toBe(
            'FILESHIP_VERSION=6.9.0-beta.4\n',
        );
    });

    it('appends the line and keeps everything else', () => {
        expect(setEnvValue('SECRET_TOKEN=abc\n# comment\n', 'FILESHIP_VERSION', '6.9.0')).toBe(
            'SECRET_TOKEN=abc\n# comment\nFILESHIP_VERSION=6.9.0\n',
        );
        expect(setEnvValue('SECRET_TOKEN=abc', 'FILESHIP_VERSION', '6.9.0')).toBe(
            'SECRET_TOKEN=abc\nFILESHIP_VERSION=6.9.0\n',
        );
    });

    it('replaces the value where it is and keeps Windows line endings', () => {
        expect(
            setEnvValue(
                'FILESHIP_VERSION=6.9.0-beta.3\r\nSECRET_TOKEN=abc\r\n',
                'FILESHIP_VERSION',
                '6.9.0-beta.4',
            ),
        ).toBe('FILESHIP_VERSION=6.9.0-beta.4\r\nSECRET_TOKEN=abc\r\n');
    });

    it('removes the line, duplicates included', () => {
        expect(
            setEnvValue(
                'SECRET_TOKEN=abc\nFILESHIP_VERSION=6.9.0\nexport FILESHIP_VERSION = 6.8.1\n',
                'FILESHIP_VERSION',
                undefined,
            ),
        ).toBe('SECRET_TOKEN=abc\n');
    });

    it('leaves comments and other keys alone', () => {
        const content = '# FILESHIP_VERSION=6.8.1\nMY_FILESHIP_VERSION=1\n';

        expect(setEnvValue(content, 'FILESHIP_VERSION', undefined)).toBe(content);
    });

    it('returns the same text when nothing changes', () => {
        const content = 'SECRET_TOKEN=abc\nFILESHIP_VERSION=6.9.0\n';

        expect(setEnvValue(content, 'FILESHIP_VERSION', '6.9.0')).toBe(content);
        expect(setEnvValue('SECRET_TOKEN=abc\n', 'FILESHIP_VERSION', undefined)).toBe(
            'SECRET_TOKEN=abc\n',
        );
    });
});
