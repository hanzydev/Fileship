import type { WebhookDeliveryStatus, WebhookFormat } from '#shared/prisma/enums';

export type WebhookDeliveryData = {
    id: string;
    event: string;
    status: WebhookDeliveryStatus;
    attempts: number;
    statusCode: number | null;
    error: string | null;
    detail: string | null;
    duration: number | null;
    nextAttemptAt: string;
    createdAt: string;
    finishedAt: string | null;
};

export type WebhookData = {
    id: string;
    name: string;
    host: string;
    format: WebhookFormat;
    events: string[];
    enabled: boolean;
    hasSecret: boolean;
    failureCount: number;
    disabledAt: string | null;
    createdAt: string;
    lastDelivery: WebhookDeliveryData | null;
};
