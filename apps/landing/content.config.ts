import { fileURLToPath } from 'node:url';

import { defineCollection, defineContentConfig, z } from '@nuxt/content';

const docsDirectory = fileURLToPath(new URL('../../docs', import.meta.url));

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
    },
});
