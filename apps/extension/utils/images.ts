const toBlob = (url: string) => fetch(url).then((response) => response.blob());

const fetchInPage = async (src: string, tabId: number, frameId: number) => {
    const [injection] = await browser.scripting
        .executeScript({
            target: { tabId, frameIds: [frameId] },
            args: [src],
            func: (source: string) =>
                fetch(source)
                    .then((response) => (response.ok ? response.blob() : null))
                    .then(
                        (blob) =>
                            blob &&
                            new Promise<string>((resolve) => {
                                const reader = new FileReader();
                                reader.onload = () => resolve(reader.result as string);
                                reader.readAsDataURL(blob);
                            }),
                    )
                    .catch(() => null),
        })
        .catch(() => []);

    if (!injection?.result) throw new ExtensionError('errors.imageDownloadFailed');
    return toBlob(injection.result);
};

export const fetchImage = (
    src: string,
    frame: { url?: string; tabId?: number; frameId?: number },
): Promise<Blob> => {
    const url = new URL(src);

    if (url.protocol === 'data:') return toBlob(src);

    const isPageOrigin =
        url.protocol === 'blob:' || (!!frame.url && url.origin === new URL(frame.url).origin);

    if (isPageOrigin && frame.tabId !== undefined)
        return fetchInPage(src, frame.tabId, frame.frameId ?? 0);

    return browser.permissions.request({ origins: [`${url.origin}/*`] }).then(async (granted) => {
        if (!granted) throw new ExtensionError('errors.imageAccessDenied', { origin: url.host });

        const response = await fetch(src, { credentials: 'omit' }).catch(() => null);
        if (!response?.ok) throw new ExtensionError('errors.imageDownloadFailed');

        return response.blob();
    });
};

const safeDecode = (value: string) => {
    try {
        return decodeURIComponent(value);
    } catch {
        return value;
    }
};

export const imageName = (src: string) => {
    const url = new URL(src);
    const segment = ['http:', 'https:'].includes(url.protocol)
        ? url.pathname.split('/').pop()!
        : '';

    return safeDecode(segment) || 'image';
};

export const withImageExtension = (name: string, type: string) => {
    const subtype = type.startsWith('image/') ? type.slice(6).split('+')[0] : '';
    if (!subtype) return name;

    return `${name.replace(/\.[^.]+$/, '')}.${subtype === 'jpeg' ? 'jpg' : subtype}`;
};

export const screenshotName = () => {
    const date = new Date();
    const pad = (value: number) => String(value).padStart(2, '0');

    return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}_${pad(date.getHours())}-${pad(date.getMinutes())}-${pad(date.getSeconds())}.png`;
};

export const captureTab = async (windowId: number) => {
    const dataUrl = await browser.tabs
        .captureVisibleTab(windowId, { format: 'png' })
        .catch(() => null);

    if (!dataUrl) throw new ExtensionError('errors.captureNotAllowed');
    return toBlob(dataUrl);
};

export const cropImage = async (image: Blob, rect: AreaRect, viewportWidth: number) => {
    const bitmap = await createImageBitmap(image);
    const scale = bitmap.width / viewportWidth;
    const x = Math.max(0, Math.round(rect.x * scale));
    const y = Math.max(0, Math.round(rect.y * scale));
    const width = Math.max(1, Math.min(bitmap.width - x, Math.round(rect.width * scale)));
    const height = Math.max(1, Math.min(bitmap.height - y, Math.round(rect.height * scale)));

    const canvas = new OffscreenCanvas(width, height);
    canvas.getContext('2d')!.drawImage(bitmap, x, y, width, height, 0, 0, width, height);
    bitmap.close();

    return canvas.convertToBlob({ type: 'image/png' });
};
