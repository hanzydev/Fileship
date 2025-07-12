import { randomUUID } from 'node:crypto';
import { existsSync, readFileSync, writeFileSync } from 'node:fs';

import { join } from 'pathe';

import { Telemetry } from '@fileship/telemetry';

const projectIdFilePath = join(dataDirectory, '.fileship_project_id');
if (!existsSync(projectIdFilePath)) writeFileSync(projectIdFilePath, randomUUID());

export const telemetry = new Telemetry({
    enabled:
        (process.env.TELEMETRY_ENABLED || 'true') === 'true' &&
        process.env.NODE_ENV === 'production',
    projectId: readFileSync(projectIdFilePath, 'utf-8'),
});
