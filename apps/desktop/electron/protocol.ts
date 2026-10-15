import { existsSync, statSync } from 'node:fs';
import { join, normalize, sep } from 'node:path';
import { pathToFileURL } from 'node:url';

import { app, net, protocol } from 'electron';

export const registerAppScheme = () =>
    protocol.registerSchemesAsPrivileged([
        {
            scheme: 'app',
            privileges: { standard: true, secure: true, supportFetchAPI: true },
        },
    ]);

export const handleAppProtocol = () => {
    const root = join(app.getAppPath(), '.output', 'public');
    const fallback = join(root, '200.html');

    protocol.handle('app', (request) => {
        const { pathname } = new URL(request.url);
        const path = normalize(join(root, decodeURIComponent(pathname)));

        const isFile =
            path.startsWith(`${root}${sep}`) && existsSync(path) && statSync(path).isFile();

        return net.fetch(pathToFileURL(isFile ? path : fallback).toString());
    });
};
