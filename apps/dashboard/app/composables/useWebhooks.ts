import type { WebhookData } from '~/types/webhooks';

export const useWebhooks = () => {
    return useState<WebhookData[] | null>('webhooks', () => null);
};
