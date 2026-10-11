import consola from 'consola';

export default defineNitroPlugin(async () => {
    const settings = await getSettings();

    setInferenceThreads(settings.aiInferenceThreads);

    if (!settings.aiEnabled) {
        consola.warn('AI is disabled, the queue runner is idle until it is enabled.');
    }

    const intervalMs = 1_000;
    let running = false;

    const tick = async () => {
        if (running || !appState.ready) return;
        running = true;

        try {
            if (!(await isAiEnabled())) return;

            await runTask('ai:processQueue');
        } catch (error: any) {
            consola.error(`AI queue runner failed: ${error?.message ?? error}`);
        } finally {
            running = false;
        }
    };

    await tick();
    setInterval(tick, intervalMs);
});
