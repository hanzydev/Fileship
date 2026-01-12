import consola from 'consola';

export default defineNitroPlugin(async () => {
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
