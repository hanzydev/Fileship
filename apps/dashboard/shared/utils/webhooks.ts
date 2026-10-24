import { WebhookFormat } from '#shared/prisma/enums';

export const WEBHOOK_EVENTS = {
    'file.uploaded': {
        label: 'webhooks.events.fileUploaded',
        description: 'webhooks.events.fileUploadedDescription',
        permission: 'files',
    },
    'file.deleted': {
        label: 'webhooks.events.fileDeleted',
        description: 'webhooks.events.fileDeletedDescription',
        permission: 'files',
    },
    'storage.almostFull': {
        label: 'webhooks.events.storageAlmostFull',
        description: 'webhooks.events.storageAlmostFullDescription',
        permission: 'files',
    },
    'link.created': {
        label: 'webhooks.events.linkCreated',
        description: 'webhooks.events.linkCreatedDescription',
        permission: 'links',
    },
    'note.created': {
        label: 'webhooks.events.noteCreated',
        description: 'webhooks.events.noteCreatedDescription',
        permission: 'notes',
    },
} as const;

export type WebhookEvent = keyof typeof WEBHOOK_EVENTS;

export const WEBHOOK_FORMATS = {
    [WebhookFormat.Discord]: { label: 'Discord', description: 'webhooks.formats.discord' },
    [WebhookFormat.Slack]: { label: 'Slack', description: 'webhooks.formats.slack' },
    [WebhookFormat.Json]: { label: 'JSON', description: 'webhooks.formats.json' },
};

export const WEBHOOK_DOCS_URL = 'https://github.com/hanzydev/Fileship/blob/main/docs/webhooks.md';

export const WEBHOOK_LIMIT = 10;
export const WEBHOOK_HISTORY = 20;
export const WEBHOOK_FAILURE_LIMIT = 5;
export const STORAGE_ALERT_PERCENT = 90;
