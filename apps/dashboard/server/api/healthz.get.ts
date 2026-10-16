import pkg from '../../../../package.json';

const getBusy = defineCachedFunction(
    async () => !!(await prisma.user.count({ where: { backupRestoreState: { not: null } } })),
    { maxAge: 5, name: 'healthz-busy', getKey: () => 'busy' },
);

export default defineEventHandler(async (event) => {
    if (!appState.ready) setResponseStatus(event, 503);

    return {
        status: appState.ready ? 'ok' : 'starting',
        version: pkg.version,
        ready: appState.ready,
        busy: await getBusy(),
    };
});
