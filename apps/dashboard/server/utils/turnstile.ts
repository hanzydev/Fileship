export const verifyTurnstile = async (token?: string) => {
    const { turnstileSiteKey, turnstileSecretKey } = await getSettings();
    if (!turnstileSiteKey || !turnstileSecretKey) return true;
    if (!token) return false;

    const result = await $fetch<{ success: boolean }>(
        'https://challenges.cloudflare.com/turnstile/v0/siteverify',
        {
            method: 'POST',
            body: new URLSearchParams({ secret: turnstileSecretKey, response: token }),
        },
    ).catch(() => null);

    return !!result?.success;
};
