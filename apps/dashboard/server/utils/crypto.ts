import { createCipheriv, createDecipheriv, randomBytes } from 'node:crypto';
import { promises as fsp } from 'node:fs';

import { join } from 'pathe';

let key: Promise<Buffer> | undefined;

const getKey = () =>
    (key ??= (async () => {
        const keyPath = join(dataDirectory, 'secret.key');

        await fsp
            .writeFile(keyPath, randomBytes(32), { flag: 'wx', mode: 0o600 })
            .catch(() => null);

        return fsp.readFile(keyPath);
    })());

export const encrypt = async (value: string) => {
    const iv = randomBytes(12);
    const cipher = createCipheriv('aes-256-gcm', await getKey(), iv);
    const encrypted = Buffer.concat([cipher.update(value, 'utf8'), cipher.final()]);

    return [iv, cipher.getAuthTag(), encrypted].map((part) => part.toString('base64url')).join('.');
};

export const decrypt = async (value: string) => {
    const [iv, tag, encrypted] = value.split('.').map((part) => Buffer.from(part, 'base64url'));
    const decipher = createDecipheriv('aes-256-gcm', await getKey(), iv!);

    decipher.setAuthTag(tag!);

    return Buffer.concat([decipher.update(encrypted!), decipher.final()]).toString('utf8');
};
