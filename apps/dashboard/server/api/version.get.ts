import pkg from '../../../../package.json';

const repoUrl = 'hanzydev/Fileship';

const cachedIsLatest = defineCachedFunction(
    async () => {
        try {
            const release = await $fetch<{ tag_name: string }>(
                `https://api.github.com/repos/${repoUrl}/releases/latest`,
            );
            return pkg.version === release.tag_name.slice(1);
        } catch {
            return false;
        }
    },
    { maxAge: 1_800 },
);

export default defineEventHandler(async () => {
    return {
        number: pkg.version,
        url: `https://github.com/${repoUrl}/releases/tag/v${pkg.version}`,
        latest: await cachedIsLatest(),
    };
});
