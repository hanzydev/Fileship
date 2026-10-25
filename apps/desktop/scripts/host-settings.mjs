import { mkdirSync, writeFileSync } from 'node:fs';
import { join } from 'node:path';

import { z } from 'zod';

import { settingsSchema } from '../shared/settings.ts';
import { root } from './msvc.mjs';

const output = join(root, 'build', 'host');

mkdirSync(output, { recursive: true });

writeFileSync(join(output, 'settings-defaults.json'), JSON.stringify(settingsSchema.parse({})));

writeFileSync(
    join(output, 'settings-schema.json'),
    JSON.stringify(z.toJSONSchema(settingsSchema, { io: 'input', unrepresentable: 'any' })),
);
