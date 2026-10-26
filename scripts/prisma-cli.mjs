import { cp, mkdir } from 'node:fs/promises';
import { createRequire } from 'node:module';
import { dirname, join } from 'node:path';

const [source, target] = process.argv.slice(2);

const require = createRequire(join(source, 'package.json'));
const { nodeFileTrace } = require('@vercel/nft');

const { fileList } = await nodeFileTrace(
    ['prisma/build/index.js', 'prisma/config', 'dotenv/config'].map((id) => require.resolve(id)),
    { base: source },
);

const files = [
    ...fileList,
    'node_modules/prisma/package.json',
    'node_modules/dotenv/package.json',
    'node_modules/@prisma/engines',
];

for (const file of files) {
    await mkdir(dirname(join(target, file)), { recursive: true });
    await cp(join(source, file), join(target, file), { recursive: true });
}
