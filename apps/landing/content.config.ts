import { fileURLToPath } from 'node:url';

import { defineCollection, defineContentConfig } from '@nuxt/content';

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
        }),
    },
});
