import type { Instance } from '~~/generated/prisma/client';

export const serializeInstance = (instance: Instance) => {
    const report = instance.report as StoredReport;

    return {
        id: instance.id,
        version: instance.version,
        users: instance.users,
        files: instance.files,
        storage: Number(instance.storage),
        platform: report.runtime?.platform ?? null,
        arch: report.runtime?.arch ?? null,
        docker: report.runtime?.docker ?? null,
        firstSeenAt: instance.firstSeenAt,
        lastSeenAt: instance.lastSeenAt,
    };
};
