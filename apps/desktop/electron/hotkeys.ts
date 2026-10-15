import { execFile } from 'node:child_process';
import { promisify } from 'node:util';

import { globalShortcut } from 'electron';

import type { HotkeyStatus } from '../shared/ipc';
import { HOTKEY_ACTIONS, type HotkeyAction } from '../shared/settings';
import { runAction } from './actions';
import { getSettings } from './store';

const run = promisify(execFile);

const KEYBOARD_KEY = 'HKCU\\Control Panel\\Keyboard';
const SNIPPING_VALUE = 'PrintScreenKeyForSnippingEnabled';

let status: HotkeyStatus = {};

export const getHotkeyStatus = () => status;

export const registerHotkeys = () => {
    globalShortcut.unregisterAll();
    status = {};

    const { hotkeys, hotkeysEnabled } = getSettings();
    if (!hotkeysEnabled) return status;

    for (const action of HOTKEY_ACTIONS) {
        const accelerator = hotkeys[action];
        if (!accelerator) continue;

        try {
            const isRegistered = globalShortcut.register(accelerator, () =>
                runAction(action as HotkeyAction),
            );

            status[action] = isRegistered ? 'ok' : 'conflict';
        } catch {
            status[action] = 'invalid';
        }
    }

    return status;
};

export const isSnippingToolKeyEnabled = async () => {
    try {
        const { stdout } = await run('reg', ['query', KEYBOARD_KEY, '/v', SNIPPING_VALUE]);
        return !/0x0\s*$/m.test(stdout);
    } catch {
        return true;
    }
};

export const setSnippingToolKey = async (enabled: boolean) => {
    await run('reg', [
        'add',
        KEYBOARD_KEY,
        '/v',
        SNIPPING_VALUE,
        '/t',
        'REG_DWORD',
        '/d',
        enabled ? '1' : '0',
        '/f',
    ]);

    return registerHotkeys();
};
