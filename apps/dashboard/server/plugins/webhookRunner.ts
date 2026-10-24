import { randomUUID } from 'node:crypto';
import { hostname } from 'node:os';

import consola from 'consola';

const CONCURRENCY = 4;

export default defineNitroPlugin(() => {
    const workerId = `${hostname()}-${process.pid}-${randomUUID().slice(0, 8)}`;

    let active = 0;
    let filling = false;

    const fill = async () => {
        if (filling || !appState.ready) return;
        filling = true;

        try {
            while (active < CONCURRENCY) {
                const delivery = await claimWebhookDelivery(workerId);
                if (!delivery) break;

                active++;
                runWebhookDelivery(delivery, workerId)
                    .catch((error) =>
                        consola.error(`Webhook delivery failed: ${error?.message ?? error}`),
                    )
                    .finally(() => {
                        active--;
                        fill();
                    });
            }
        } catch (error: any) {
            consola.error(`Webhook runner failed: ${error?.message ?? error}`);
        } finally {
            filling = false;
        }
    };

    setInterval(fill, 2_000);
});
