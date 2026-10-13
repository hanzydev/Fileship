import { readdir, stat } from 'node:fs/promises';

import { join } from 'pathe';

import { AIJobStatus } from '#shared/prisma/enums';

const directorySize = async (path: string): Promise<number> => {
    const entries = await readdir(path, { withFileTypes: true }).catch(() => []);
    const sizes = await Promise.all(
        entries.map((entry) =>
            entry.isDirectory()
                ? directorySize(join(path, entry.name))
                : stat(join(path, entry.name)).then((info) => info.size),
        ),
    );

    return sizes.reduce((sum, size) => sum + size, 0);
};

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const counts = await prisma.aIJob.groupBy({ by: ['status'], _count: true });
    const count = (status: AIJobStatus) => counts.find((row) => row.status === status)?._count ?? 0;

    return {
        pending: count(AIJobStatus.Pending),
        processing: count(AIJobStatus.Processing),
        failed: count(AIJobStatus.Failed),
        modelCacheSize: await directorySize(join(dataDirectory, 'models')),
    };
});
