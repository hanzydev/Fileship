export default defineNuxtPlugin(() => {
    const bridge = window.fileshipDesktop;
    if (!bridge) return;

    const { isDesktop, downloads, downloadsStarted, isHarborAvailable } = useDesktopApp();

    onNuxtReady(async () => {
        bridge.downloads.onChange((value) => {
            const known = new Set(downloads.value.map((download) => download.id));

            if (value.some((download) => !known.has(download.id))) downloadsStarted.value++;
            downloads.value = value;
        });

        bridge.harbor.onChange((value) => {
            isHarborAvailable.value = value;
        });

        [downloads.value, isHarborAvailable.value] = await Promise.all([
            bridge.downloads.list(),
            bridge.harbor.isAvailable(),
        ]);

        isDesktop.value = true;
    });
});
