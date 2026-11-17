import semver from 'semver';

import type { Channel } from './releases';

export const versionToPin = (channel: Channel, version: string) =>
    channel === 'experimental' || semver.prerelease(version) ? version : undefined;

export const setEnvValue = (content: string, key: string, value: string | undefined) => {
    const eol = content.includes('\r\n') ? '\r\n' : '\n';
    const pattern = new RegExp(`^\\s*(?:export\\s+)?${key}\\s*=`);
    const lines = content.split(/\r?\n/);
    const first = lines.findIndex((line) => pattern.test(line));

    if (first === -1) {
        if (value === undefined) return content;

        return `${content && !content.endsWith('\n') ? content + eol : content}${key}=${value}${eol}`;
    }

    return lines
        .flatMap((line, index) => {
            if (index === first) return value === undefined ? [] : [`${key}=${value}`];
            return pattern.test(line) ? [] : [line];
        })
        .join(eol);
};
