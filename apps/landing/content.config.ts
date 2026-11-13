import { fileURLToPath } from 'node:url';

import { defineCollection, defineContentConfig, z } from '@nuxt/content';

import { docsVersions } from './docsVersions.ts';

const docsDirectory = fileURLToPath(new URL('../../docs', import.meta.url));
const releasesDirectory = fileURLToPath(new URL('./.data/releases', import.meta.url));

const docsCollection = (key: 'stable' | 'experimental', prefix: string) =>
    defineCollection({
        type: 'page',
        source: {
            cwd:
                docsVersions.extracted === key
                    ? fileURLToPath(new URL(`./.data/docs/${docsVersions[key]}`, import.meta.url))
                    : docsDirectory,
            include: '**/*.md',
            prefix,
        },
        schema: z.object({
            rawbody: z.string(),
        }),
    });

export default defineContentConfig({
    collections: {
        docs: docsCollection('stable', '/docs'),
        docsExperimental: docsCollection('experimental', '/docs/experimental'),
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
