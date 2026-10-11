import pkg from '../../../../package.json';

export default defineEventHandler(async (event) => {
    if (!appState.ready) setResponseStatus(event, 503);

    return {
        status: appState.ready ? 'ok' : 'starting',
        version: pkg.version,
        ready: appState.ready,
        busy: !!(await prisma.user.count({ where: { backupRestoreState: { not: null } } })),
    };
});
