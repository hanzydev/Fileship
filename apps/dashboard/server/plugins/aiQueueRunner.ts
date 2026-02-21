import consola from 'consola';

export default defineNitroPlugin(async () => {
    if (!AI_ENABLED) {
        consola.warn(
            'AI is disabled, queue runner will not start. Set NUXT_PUBLIC_AI_ENABLED=true to enable it.',
        );
        return;
    }

    const intervalMs = 1_000;

    const tick = async () => {
        try {
            await runTask('ai:processQueue');
        } catch (error: any) {
            consola.error(`AI queue runner failed: ${error?.message ?? error}`);
        }
    };

    await tick();
    setInterval(tick, intervalMs);
});
