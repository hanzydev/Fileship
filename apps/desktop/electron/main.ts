import { app, BrowserWindow, dialog } from 'electron';
import log from 'electron-log/main';

import { openDashboard } from './dashboard';
import { connectHost } from './host';
import { loadHostState, registerIpc } from './ipc';
import { handleAppProtocol, registerAppScheme } from './protocol';
import { markQuitting, showMainWindow } from './windows';

log.initialize();
log.errorHandler.startCatching();

const DEBUG_SWITCHES = ['remote-debugging-port', 'remote-debugging-pipe'];
const IDLE_QUIT_MS = 60_000;

let idleTimer: NodeJS.Timeout | null = null;

if (app.isPackaged && DEBUG_SWITCHES.some((name) => app.commandLine.hasSwitch(name))) {
    app.exit(1);
}

if (!app.requestSingleInstanceLock()) {
    app.exit(0);
}

app.setAppUserModelId(app.isPackaged ? 'dev.hanzy.fileship' : 'dev.hanzy.fileship.dev');
registerAppScheme();

const argumentValue = (argv: string[], name: string) => {
    const index = argv.indexOf(name);
    return index === -1 ? null : (argv[index + 1] ?? null);
};

const openFromArguments = (argv: string[]) => {
    const dashboard = argumentValue(argv, '--dashboard');
    if (dashboard) return openDashboard(dashboard);

    showMainWindow(argumentValue(argv, '--route') ?? undefined);
};

const quit = () => {
    markQuitting();
    app.quit();
};

app.on('second-instance', (_, argv) => openFromArguments(argv));

app.on('browser-window-created', () => {
    if (idleTimer) clearTimeout(idleTimer);
    idleTimer = null;
});

app.on('window-all-closed', () => {
    idleTimer = setTimeout(() => {
        if (BrowserWindow.getAllWindows().length === 0) quit();
    }, IDLE_QUIT_MS);
});

app.on('web-contents-created', (_, contents) => {
    if (app.isPackaged) contents.on('devtools-opened', () => contents.closeDevTools());
});

app.whenReady().then(async () => {
    if (!process.env.DESKTOP_RENDERER_URL) handleAppProtocol();

    try {
        await connectHost(quit);
        await loadHostState();
    } catch (error) {
        log.error('Could not reach the Fileship host', error);
        dialog.showErrorBox('Fileship', String((error as Error).message ?? error));
        app.exit(1);
        return;
    }

    registerIpc();
    openFromArguments(process.argv);
});
