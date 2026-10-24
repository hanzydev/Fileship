let creating: Promise<void> | null = null;

const ensureOffscreenDocument = async () => {
    const url = browser.runtime.getURL('/offscreen.html');
    const contexts = await browser.runtime.getContexts({
        contextTypes: ['OFFSCREEN_DOCUMENT'],
        documentUrls: [url],
    });

    if (contexts.length) return;

    creating ??= browser.offscreen
        .createDocument({
            url,
            reasons: ['CLIPBOARD'],
            justification: 'Copy the links of uploaded files and short links.',
        })
        .finally(() => (creating = null));

    await creating;
};

export const copyText = async (text: string) => {
    if (import.meta.env.FIREFOX) return navigator.clipboard.writeText(text);

    await ensureOffscreenDocument();
    if (!(await sendMessage<boolean>({ type: 'copy', text }))) throw new Error('Copy failed');
};
