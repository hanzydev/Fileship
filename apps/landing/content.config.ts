import { fileURLToPath } from 'node:url';

import { defineCollection, defineContentConfig, z } from '@nuxt/content';

const docsDirectory = fileURLToPath(new URL('../../docs', import.meta.url));
const releasesDirectory = fileURLToPath(new URL('./.data/releases', import.meta.url));

export default defineContentConfig({
    collections: {
        docs: defineCollection({
            type: 'page',
            source: {
                cwd: docsDirectory,
                include: '**/*.md',
                prefix: '/docs',
            },
            schema: z.object({
                rawbody: z.string(),
            }),
        }),
        releases: defineCollection({
            type: 'page',
            source: {
                cwd: releasesDirectory,
                include: '*.md',
                prefix: '/releases',
            },
            schema: z.object({
                version: z.string(),
                major: z.number(),
                order: z.number(),
                date: z.string(),
                compare: z.string(),
                desktop: z.string(),
            }),
        }),
    },
});
