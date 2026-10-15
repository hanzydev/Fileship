import { clipboard, ClipboardItem, nativeImage } from 'electron';
import log from 'electron-log/main';

const FILE_NAME_TYPE = 'electron application/osclipboard;format="FileNameW"';
const RETRY_DELAYS = [0, 100, 200];
const HISTORY_GAP_MS = 1000;

let lastImageWriteAt = 0;

const wait = (ms: number) => new Promise((resolve) => setTimeout(resolve, Math.max(0, ms)));

const withRetry = async (label: string, write: () => Promise<void>) => {
    for (const [attempt, delay] of RETRY_DELAYS.entries()) {
        await wait(delay);

        try {
            await write();
            return true;
        } catch (error) {
            if (attempt === RETRY_DELAYS.length - 1) log.warn(`Could not copy the ${label}`, error);
        }
    }

    return false;
};

const readType = async (type: string) => {
    for (const item of await clipboard.read()) {
        if (!item.types.includes(type)) continue;

        const blob = (await item.getType(type)) as Blob;
        return Buffer.from(await blob.arrayBuffer());
    }

    return null;
};

export const writeText = async (text: string) => {
    await wait(lastImageWriteAt + HISTORY_GAP_MS - Date.now());
    return withRetry('text', () => clipboard.writeText(text));
};

export const readText = () => clipboard.readText();

export const writeImage = async (image: Electron.NativeImage) => {
    const isCopied = await withRetry('image', () =>
        clipboard.write([
            new ClipboardItem({
                'image/png': new Blob([new Uint8Array(image.toPNG())], { type: 'image/png' }),
            }),
        ]),
    );

    if (isCopied) lastImageWriteAt = Date.now();
    return isCopied;
};

export const readImage = async () => {
    const png = await readType('image/png');
    return png ? nativeImage.createFromBuffer(png) : null;
};

export const readFilePath = async () => {
    if (!(await clipboard.has(FILE_NAME_TYPE))) return null;

    const raw = await readType(FILE_NAME_TYPE);
    return raw?.toString('utf16le').split('\0')[0] || null;
};
