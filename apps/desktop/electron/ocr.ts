import log from 'electron-log/main';

import { handleUnauthorized } from './account';
import { api, ApiError } from './api';
import { type Bitmap, encodePng } from './capture';
import { writeText } from './clipboard';
import { openRegionCapture } from './overlay';
import { notify } from './windows';

const OCR_ERRORS: Record<number, [string, string]> = {
    403: ['Text capture is off', 'AI features are turned off on your Fileship.'],
    404: ['Text capture is not available', 'Update your Fileship to capture text.'],
    429: ['Too many text captures', 'Wait a minute and try again.'],
};

const recognize = async (image: Bitmap) => {
    try {
        const form = new FormData();
        form.append(
            'image',
            new Blob([new Uint8Array(await encodePng(image))], { type: 'image/png' }),
            'capture.png',
        );

        const { text } = await api<{ text: string }>('/api/ai/ocr', {
            method: 'POST',
            body: form,
            timeout: 120_000,
        });

        if (!text.trim()) return notify('No text found', 'Try selecting a larger area.');

        await writeText(text);
        notify('Text copied', text.length > 200 ? `${text.slice(0, 200)}…` : text);
    } catch (error: any) {
        if (error instanceof ApiError && error.status === 401) return handleUnauthorized();

        const known = error instanceof ApiError ? OCR_ERRORS[error.status] : undefined;
        if (known) return notify(...known);

        log.warn('Text capture failed', error);
        notify('Could not capture text', error.message);
    }
};

export const captureText = () =>
    openRegionCapture('ocr', null, (image) => {
        recognize(image);
    });
