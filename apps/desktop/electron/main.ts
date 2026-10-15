import { app, globalShortcut } from 'electron';
import log from 'electron-log/main';

import { registerHotkeys } from './hotkeys';
import { registerIpc } from './ipc';
import { createOverlays, watchDisplays } from './overlay';
import { handleAppProtocol, registerAppScheme } from './protocol';
import { createTray } from './tray';
import { setBusyCheck, startUpdater } from './updater';
import { isUploading } from './upload';
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

app.on('will-quit', () => globalShortcut.unregisterAll());

app.whenReady().then(() => {
    if (!process.env.DESKTOP_RENDERER_URL) handleAppProtocol();

    registerIpc();
    createMainWindow(!process.argv.includes('--hidden'));
    createTray();
    createOverlays();
    watchDisplays();
    registerHotkeys();
    setBusyCheck(isUploading);
    startUpdater();
});
