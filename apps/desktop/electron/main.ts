import { app } from 'electron';
import log from 'electron-log/main';

import { registerIpc } from './ipc';
import { handleAppProtocol, registerAppScheme } from './protocol';
import { createTray } from './tray';
import { startUpdater } from './updater';
import { createMainWindow, showMainWindow } from './windows';

log.initialize();
log.errorHandler.startCatching();

if (!app.requestSingleInstanceLock()) {
    app.exit(0);
}

app.setAppUserModelId('dev.hanzy.fileship');
registerAppScheme();

app.on('second-instance', () => showMainWindow());

app.on('window-all-closed', () => {});

app.whenReady().then(() => {
    if (!process.env.DESKTOP_RENDERER_URL) handleAppProtocol();

    registerIpc();
    createMainWindow(!process.argv.includes('--hidden'));
    createTray();
    startUpdater();
});
