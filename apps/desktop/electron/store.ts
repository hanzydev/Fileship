import { mkdirSync, readFileSync, renameSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';

import { app, safeStorage } from 'electron';
import { z } from 'zod';

import type { AccountUser } from '../shared/ipc';
import { type Settings, settingsSchema } from '../shared/settings';

const userSchema = z.object({
    id: z.string(),
    username: z.string(),
    avatar: z.string().nullable(),
    permissions: z.array(z.enum(['Admin', 'UploadFiles', 'TakeNotes', 'ShortenLinks'])),
    superAdmin: z.boolean(),
});

const storeSchema = z.object({
    instanceUrl: z.string().nullable().default(null),
    siteName: z.string().default('Fileship'),
    token: z.string().nullable().default(null),
    uploadPassword: z.string().nullable().default(null),
    user: userSchema.nullable().default(null),
    settings: settingsSchema.prefault({}),
    driveRoot: z.string().nullable().default(null),
    ffmpegProbe: z
        .object({
            key: z.string(),
            version: z.string(),
            ddagrab: z.boolean(),
            encoders: z.array(z.string()),
        })
        .nullable()
        .catch(null)
        .default(null),
});

type StoreData = z.infer<typeof storeSchema>;

const storePath = () => join(app.getPath('userData'), 'settings.json');

let data: StoreData | null = null;

const encrypt = (value: string) => safeStorage.encryptString(value).toString('base64');

const decrypt = (value: string | null) => {
    if (!value) return null;

    try {
        return safeStorage.decryptString(Buffer.from(value, 'base64'));
    } catch {
        return null;
    }
};

const withoutPassword = (settings: Settings): Settings => ({
    ...settings,
    upload: { ...settings.upload, password: '' },
});

const migratePassword = (stored: StoreData) => {
    const password = stored.settings.upload.password;
    if (!password || !safeStorage.isEncryptionAvailable()) return false;

    stored.uploadPassword = encrypt(password);
    stored.settings = withoutPassword(stored.settings);
    return true;
};

const load = (): StoreData => {
    if (data) return data;

    try {
        data = storeSchema.parse(JSON.parse(readFileSync(storePath(), 'utf-8')));
    } catch {
        data = storeSchema.parse({});
    }

    if (migratePassword(data)) save();

    return data;
};

const save = () => {
    const path = storePath();
    mkdirSync(dirname(path), { recursive: true });
    writeFileSync(`${path}.tmp`, JSON.stringify(data, null, 2));
    renameSync(`${path}.tmp`, path);
};

export const getStore = () => load();

export const updateStore = (patch: Partial<StoreData>) => {
    data = { ...load(), ...patch };
    save();
    return data;
};

export const getSettings = (): Settings => {
    const { settings, uploadPassword } = load();
    if (settings.upload.password || !uploadPassword) return settings;

    return { ...settings, upload: { ...settings.upload, password: decrypt(uploadPassword) ?? '' } };
};

export const updateSettings = (patch: Partial<Settings>) => {
    const settings = settingsSchema.parse({ ...getSettings(), ...patch });
    const { password } = settings.upload;

    updateStore({
        settings: withoutPassword(settings),
        uploadPassword: password ? encrypt(password) : null,
    });

    return settings;
};

export const getToken = () => decrypt(load().token);

export const setSession = (session: {
    instanceUrl: string;
    siteName: string;
    token: string;
    user: AccountUser;
}) =>
    updateStore({
        ...session,
        token: encrypt(session.token),
    });

export const clearSession = () => updateStore({ token: null, user: null });
