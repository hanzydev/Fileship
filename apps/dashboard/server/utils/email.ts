import type { H3Event } from 'h3';
import { createTransport } from 'nodemailer';
import { z } from 'zod';

import { pluralIndex } from '@fileship/ui/i18n/plural';
import { createCoreContext, translate as translateMessage } from '@intlify/core';

type Params = Record<string, string | number>;

type EmailTemplate = {
    paragraphs: string[];
    notes?: string[];
};

const emailTemplates = {
    test: { paragraphs: ['body'] },
    verifyEmail: { paragraphs: ['body'], notes: ['expires', 'ignore'] },
    resetPassword: { paragraphs: ['body'], notes: ['expires', 'ignore'] },
    invite: { paragraphs: ['body'], notes: ['expires', 'ignore'] },
    storageWarning: { paragraphs: ['body', 'freeUp'] },
} satisfies Record<string, EmailTemplate>;

const catalogs: Record<Language, () => Promise<{ default: object }>> = {
    en: () => import('~~/i18n/locales/en.json'),
    tr: () => import('~~/i18n/locales/tr.json'),
    de: () => import('~~/i18n/locales/de.json'),
    es: () => import('~~/i18n/locales/es.json'),
    fr: () => import('~~/i18n/locales/fr.json'),
    it: () => import('~~/i18n/locales/it.json'),
    pt: () => import('~~/i18n/locales/pt.json'),
    ru: () => import('~~/i18n/locales/ru.json'),
    az: () => import('~~/i18n/locales/az.json'),
    ja: () => import('~~/i18n/locales/ja.json'),
    ko: () => import('~~/i18n/locales/ko.json'),
    zh: () => import('~~/i18n/locales/zh.json'),
};

const escapeHtml = (value: string) =>
    value.replace(/[&<>"']/g, (char) => `&#${char.charCodeAt(0)};`);

export const emailAddressSchema = z.string().trim().toLowerCase().pipe(z.email().max(254));

export const isEmailConfigured = (settings: InstanceSettings) =>
    !!(settings.smtpHost && settings.smtpFrom && settings.siteUrl);

export const isEmailEnabled = async () => isEmailConfigured(await getSettings());

export const emailLanguage = (user: { language: string | null }, event?: H3Event) =>
    toLanguage(user.language) ?? (event ? requestLanguage(event) : 'en');

const createTranslator = async (language: Language) => {
    const [en, messages] = await Promise.all([catalogs.en(), catalogs[language]()]);
    const context = createCoreContext({
        locale: language,
        fallbackLocale: 'en',
        messages: { en: en.default, [language]: messages.default } as any,
        pluralRules: {
            [language]: (choice: number, choicesLength: number) =>
                pluralIndex(language, choice, choicesLength),
        },
        missingWarn: false,
        fallbackWarn: false,
    });

    return (key: string, params: Params) => String(translateMessage(context as any, key, params));
};

const renderHtml = (
    language: Language,
    siteName: string,
    content: {
        subject: string;
        heading: string;
        paragraphs: string[];
        action?: { label: string; url: string };
        copyLink: string;
        notes: string[];
        footer: string;
    },
) => {
    const paragraph = (text: string, style = 'margin:0 0 16px;') =>
        `<p style="${style}">${escapeHtml(text)}</p>`;

    const action = content.action
        ? `<p style="margin:24px 0;"><a href="${escapeHtml(content.action.url)}" style="display:inline-block;padding:12px 20px;border-radius:10px;background:#4338ca;color:#ffffff;font-weight:600;text-decoration:none;">${escapeHtml(content.action.label)}</a></p>` +
          `<p style="margin:0 0 16px;font-size:14px;line-height:20px;color:#525252;">${escapeHtml(content.copyLink)}<br><a href="${escapeHtml(content.action.url)}" style="color:#4338ca;word-break:break-all;">${escapeHtml(content.action.url)}</a></p>`
        : '';

    return `<!doctype html>
<html lang="${language}" dir="ltr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="color-scheme" content="light">
<title>${escapeHtml(content.subject)}</title>
</head>
<body style="margin:0;padding:0;background:#f5f5f5;">
<div role="article" aria-roledescription="email" aria-label="${escapeHtml(content.subject)}" lang="${language}" dir="ltr" style="padding:32px 16px;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;font-size:16px;line-height:24px;color:#0a0a0a;">
<table role="presentation" width="100%" cellpadding="0" cellspacing="0" border="0" style="max-width:560px;margin:0 auto;background:#ffffff;border:1px solid #e5e5e5;border-radius:16px;">
<tr><td style="padding:32px;">
${paragraph(siteName, 'margin:0 0 24px;font-size:14px;line-height:20px;font-weight:600;color:#525252;')}
<h1 style="margin:0 0 16px;font-size:22px;line-height:30px;font-weight:600;color:#0a0a0a;">${escapeHtml(content.heading)}</h1>
${content.paragraphs.map((text) => paragraph(text)).join('\n')}
${action}
${content.notes.map((text) => paragraph(text, 'margin:0 0 8px;font-size:14px;line-height:20px;color:#525252;')).join('\n')}
</td></tr>
</table>
${paragraph(content.footer, 'margin:16px auto 0;max-width:560px;font-size:12px;line-height:18px;color:#525252;text-align:center;')}
</div>
</body>
</html>`;
};

export const sendEmail = async (options: {
    to: string;
    language: Language;
    template: keyof typeof emailTemplates;
    params?: Params;
    path?: `/${string}`;
    settings?: InstanceSettings;
}) => {
    const settings = options.settings ?? (await getSettings());
    const t = await createTranslator(options.language);
    const params = { siteName: settings.siteName, ...options.params };
    const template: EmailTemplate = emailTemplates[options.template];
    const key = (part: string) => t(`emails.${options.template}.${part}`, params);

    const content = {
        subject: key('subject'),
        heading: key('heading'),
        paragraphs: template.paragraphs.map(key),
        action: options.path && {
            label: key('action'),
            url: new URL(options.path, settings.siteUrl).href,
        },
        copyLink: t('emails.copyLink', params),
        notes: (template.notes ?? []).map(key),
        footer: t('emails.footer', params),
    };

    const text = [
        content.heading,
        ...content.paragraphs,
        content.action && `${content.action.label}: ${content.action.url}`,
        ...content.notes,
        content.footer,
    ]
        .filter(Boolean)
        .join('\n\n');

    await createTransport({
        host: settings.smtpHost,
        port: settings.smtpPort,
        secure: settings.smtpSecurity === 'tls',
        requireTLS: settings.smtpSecurity === 'starttls',
        ignoreTLS: settings.smtpSecurity === 'none',
        auth: settings.smtpUser
            ? { user: settings.smtpUser, pass: settings.smtpPassword }
            : undefined,
        connectionTimeout: 10_000,
        greetingTimeout: 10_000,
        socketTimeout: 30_000,
    }).sendMail({
        from: { name: settings.siteName, address: settings.smtpFrom },
        to: options.to,
        subject: content.subject,
        text,
        html: renderHtml(options.language, settings.siteName, content),
    });
};
