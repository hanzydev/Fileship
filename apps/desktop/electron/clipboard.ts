import { clipboard, ClipboardItem, nativeImage } from 'electron';

const FILE_NAME_TYPE = 'electron application/osclipboard;format="FileNameW"';

const readType = async (type: string) => {
    for (const item of await clipboard.read()) {
        if (!item.types.includes(type)) continue;

        const blob = (await item.getType(type)) as Blob;
        return Buffer.from(await blob.arrayBuffer());
    }

    return null;
};

export const writeText = (text: string) => clipboard.writeText(text);

export const readText = () => clipboard.readText();

export const writeImage = (image: Electron.NativeImage) =>
    clipboard.write([
        new ClipboardItem({
            'image/png': new Blob([new Uint8Array(image.toPNG())], { type: 'image/png' }),
        }),
    ]);

export const readImage = async () => {
    const png = await readType('image/png');
    return png ? nativeImage.createFromBuffer(png) : null;
};

export const readFilePath = async () => {
    if (!(await clipboard.has(FILE_NAME_TYPE))) return null;

    const raw = await readType(FILE_NAME_TYPE);
    return raw?.toString('utf16le').split('\0')[0] || null;
};
