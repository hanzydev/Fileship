import { filesize } from 'filesize';
import type { H3Event } from 'h3';
import type { core, ZodError } from 'zod';

declare module 'h3' {
    interface H3EventContext {
        t?: (key: string, params?: Record<string, unknown>) => string;
    }
}

const MESSAGE_KEY = /^[a-z][A-Za-z]*\.[\w.]+$/;

const currentEvent = () => {
    try {
        return useEvent();
    } catch {
        return null;
    }
};

export const translate = (key: string, params?: Record<string, unknown>) =>
    currentEvent()?.context.t?.(key, params) ?? key;

export const formatSize = (size: number | string | bigint) =>
    filesize(size, { locale: (currentEvent()?.context.i18n as any)?.locale || 'en' });

export const refreshTranslation = async (event: H3Event) => {
    event.context.t = await useTranslation(event);
};

export const apiError = (
    status: number,
    key: string,
    params?: Record<string, unknown>,
    data?: any,
) => createError({ status, message: translate(key, params), data });

const issueMessage = (issue: core.$ZodIssue) => {
    if (MESSAGE_KEY.test(issue.message)) return translate(issue.message);

    if (issue.code === 'too_small' || issue.code === 'too_big') {
        const limit = issue.code === 'too_small' ? issue.minimum : issue.maximum;
        const origin = ['string', 'number', 'array'].includes(issue.origin)
            ? issue.origin
            : 'number';
        return translate(`errorsCommon.validation.${issue.code}.${origin}`, {
            limit: String(limit),
        });
    }

    if (issue.code === 'invalid_format')
        return translate(
            ['url', 'email'].includes(issue.format)
                ? `errorsCommon.validation.${issue.format}`
                : 'errorsCommon.validation.format',
        );

    return translate('errorsCommon.validation.invalid');
};

export const translateIssues = <T extends ZodError | undefined>(error: T) => {
    for (const issue of error?.issues ?? []) (issue as any).message = issueMessage(issue);
    return error;
};

export const invalidBodyError = (error: ZodError) =>
    apiError(400, 'errorsCommon.invalidBody', undefined, {
        formErrors: translateIssues(error).format(),
    });
