import log from 'electron-log/main';

import { handleUnauthorized } from './account';
import { api, ApiError } from './api';
import { type Bitmap, encodePng } from './capture';
import { writeText } from './clipboard';
import { t } from './i18n';
import { openRegionCapture } from './overlay';
import { notify } from './windows';

const OCR_ERRORS: Record<number, [string, string]> = {
    403: ['tasks.textCaptureOff', 'tasks.aiOff'],
    404: ['tasks.textCaptureUnavailable', 'tasks.updateFileship'],
    429: ['tasks.tooManyTextCaptures', 'tasks.waitAMinute'],
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

        if (!text.trim()) return notify(t('tasks.noTextFound'), t('tasks.selectLargerArea'));

        await writeText(text);
        notify(t('tasks.textCopied'), text.length > 200 ? `${text.slice(0, 200)}…` : text);
    } catch (error: any) {
        if (error instanceof ApiError && error.status === 401) return handleUnauthorized();

        const known = error instanceof ApiError ? OCR_ERRORS[error.status] : undefined;
        if (known) return notify(t(known[0]), t(known[1]));

        log.warn('Text capture failed', error);
        notify(t('tasks.textCaptureFailed'), error.message);
    }
};

export const captureText = () =>
    openRegionCapture('ocr', null, (image) => {
        recognize(image);
    });
