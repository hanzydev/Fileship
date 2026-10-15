import { randomUUID } from 'node:crypto';
import { join } from 'node:path';

import { BrowserWindow, screen } from 'electron';

import type { EditorFinish, EditorImage } from '../shared/ipc';
import { type Bitmap, bitmapFromRgba } from './capture';
import { hardenWebContents, iconPath, rendererUrl, themeBackground } from './windows';

export type EditorResult =
    { action: 'cancel' } | { action: 'continue' | 'copy' | 'save'; image: Bitmap };

type Session = {
    image: EditorImage;
    window: BrowserWindow;
    resolve: (result: EditorResult) => void;
};

const sessions = new Map<string, Session>();

const settle = (id: string, result: EditorResult) => {
    const session = sessions.get(id);
    if (!session) return;

    sessions.delete(id);
    session.resolve(result);
    if (!session.window.isDestroyed()) session.window.destroy();
};

export const openEditor = (
    image: { png: Buffer; width: number; height: number },
    options: { name: string; continueLabel: string },
) =>
    new Promise<EditorResult>((resolve) => {
        const id = randomUUID();
        const size = { width: image.width, height: image.height };
        const { workArea } = screen.getDisplayNearestPoint(screen.getCursorScreenPoint());
        const width = Math.min(Math.max(size.width + 96, 1240), workArea.width);
        const height = Math.min(Math.max(size.height + 160, 640), workArea.height);

        const window = new BrowserWindow({
            x: Math.round(workArea.x + (workArea.width - width) / 2),
            y: Math.round(workArea.y + (workArea.height - height) / 2),
            width,
            height,
            minWidth: 720,
            minHeight: 480,
            show: false,
            title: 'Edit · Fileship',
            icon: iconPath(),
            autoHideMenuBar: true,
            backgroundColor: themeBackground(),
            webPreferences: {
                preload: join(__dirname, 'preload.cjs'),
                contextIsolation: true,
                sandbox: true,
                nodeIntegration: false,
            },
        });

        sessions.set(id, {
            image: {
                png: new Uint8Array(image.png),
                name: options.name,
                continueLabel: options.continueLabel,
            },
            window,
            resolve,
        });

        hardenWebContents(window);
        window.once('ready-to-show', () => {
            window.show();
            window.focus();
        });
        window.on('closed', () => settle(id, { action: 'cancel' }));
        window.loadURL(rendererUrl(`/editor?id=${id}`));
    });

export const getEditorImage = (id: string) => {
    const session = sessions.get(id);
    if (!session) throw new Error('This editor was closed.');
    return session.image;
};

export const finishEditor = (id: string, result: EditorFinish) => {
    if (result.action === 'cancel') return settle(id, { action: 'cancel' });

    settle(id, {
        action: result.action,
        image: bitmapFromRgba(result.rgba, result.width, result.height),
    });
};
