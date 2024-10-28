import pkg from '~~/package.json';

const repoUrl = 'hanzydev/Fileship';

export default defineEventHandler(async () => {
    const response = {
        number: pkg.version,
        url: `https://github.com/${repoUrl}/releases/tag/v${pkg.version}`,
    };

    try {
        const latestRelease = await $fetch<{ tag_name: string }>(
            `https://api.github.com/repos/${repoUrl}/releases/latest`,
        );

        return {
            ...response,
            latest: pkg.version === latestRelease.tag_name.slice(1),
        };
    } catch {
        return { ...response, latest: true };
    }
});
