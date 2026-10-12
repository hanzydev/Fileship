import { verify } from 'otplib';

export const verifyTotpToken = async (userId: string, secret: string, token: string) => {
    const storage = useStorage('cache');
    const key = `totpUsed:${userId}:${token}`;
    const usedAt = await storage.getItem<number>(key);

    if (usedAt && Date.now() - usedAt < 90_000) return false;

    const { valid } = await verify({ secret, token });
    if (valid) await storage.setItem(key, Date.now());

    return valid;
};
