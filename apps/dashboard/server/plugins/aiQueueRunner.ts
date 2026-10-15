import { randomUUID } from 'node:crypto';
import { hostname } from 'node:os';

import consola from 'consola';

export default defineNitroPlugin(async (nitroApp) => {
    nitroApp.hooks.hook('close', () => stopAIWorker());

    const workerId = `${hostname()}-${process.pid}-${randomUUID().slice(0, 8)}`;

    if (!(await isAiEnabled())) {
        consola.warn('AI is disabled, the queue runner is idle until it is enabled.');
    } else {
        ai.warmUp().catch((error) =>
            consola.error(`Loading AI models failed: ${error?.message ?? error}`),
        );
    }

    let active = 0;
    let filling = false;

    const fill = async () => {
        if (filling || !appState.ready) return;
        filling = true;

        try {
            const settings = await getSettings();
            if (!settings.aiEnabled) return;

            while (active < settings.aiQueueConcurrency) {
                const job = await claimAIJob(workerId);
                if (!job) break;

                active++;
                runAIJob(job, workerId).finally(() => {
                    active--;
                    fill();
                });
            }
        } catch (error: any) {
            consola.error(`AI queue runner failed: ${error?.message ?? error}`);
        } finally {
            filling = false;
        }
    };

    setInterval(fill, 2_000);
});
