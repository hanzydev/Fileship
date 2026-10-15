import { app, globalShortcut } from 'electron';
import log from 'electron-log/main';

import { registerHotkeys } from './hotkeys';
import { registerIpc } from './ipc';
import { createOverlays, watchDisplays } from './overlay';
import { handleAppProtocol, registerAppScheme } from './protocol';
import { getFfmpeg } from './recording/ffmpeg';
import { isRecording, quitRecorder, registerRecorder } from './recording/recorder';
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

app.on('will-quit', () => {
    globalShortcut.unregisterAll();
    quitRecorder();
});

app.whenReady().then(() => {
    if (!process.env.DESKTOP_RENDERER_URL) handleAppProtocol();

    registerIpc();
    registerRecorder();
    createMainWindow(!process.argv.includes('--hidden'));
    createTray();
    createOverlays();
    watchDisplays();
    registerHotkeys();
    setBusyCheck(() => isUploading() || isRecording());
    startUpdater();
    setTimeout(() => getFfmpeg(), 5000);
});
