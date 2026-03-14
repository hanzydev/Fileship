import { randomUUID } from 'node:crypto';
import { existsSync, readFileSync, writeFileSync } from 'node:fs';

import { join } from 'pathe';

import { Telemetry } from '@fileship/telemetry';

import pkg from '../../../../../package.json';

export default defineTask({
    meta: {
        name: 'telemetry:collect',
        description: 'Collects telemetry data about the system and Fileship usage.',
    },
    async run() {
        const projectIdFilePath = join(dataDirectory, '.fileship_project_id');
        if (!existsSync(projectIdFilePath)) writeFileSync(projectIdFilePath, randomUUID());

        const telemetry = new Telemetry({
            enabled:
                (process.env.TELEMETRY_ENABLED || 'true') === 'true' &&
                process.env.NODE_ENV === 'production',
            projectId: readFileSync(projectIdFilePath, 'utf-8'),
        });

        try {
            await telemetry.collectSystemInfo();

            const collectFileshipInfo = async () => {
                const [files, folders, notes, users] = await prisma.$transaction([
                    prisma.file.count(),
                    prisma.folder.count(),
                    prisma.note.count(),
                    prisma.user.count(),
                ]);

                await telemetry.collectFileshipInfo({
                    version: pkg.version,
                    files,
                    folders,
                    notes,
                    users,
                });
            };

            await collectFileshipInfo();

            return { result: 'success' };
        } catch {
            return { result: 'error' };
        }
    },
});
