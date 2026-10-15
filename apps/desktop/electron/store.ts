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
    user: userSchema.nullable().default(null),
    settings: settingsSchema.prefault({}),
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

const load = (): StoreData => {
    if (data) return data;

    try {
        data = storeSchema.parse(JSON.parse(readFileSync(storePath(), 'utf-8')));
    } catch {
        data = storeSchema.parse({});
    }

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

export const getSettings = (): Settings => load().settings;

export const updateSettings = (patch: Partial<Settings>) =>
    updateStore({ settings: settingsSchema.parse({ ...load().settings, ...patch }) }).settings;

export const getToken = () => {
    const token = load().token;
    if (!token) return null;

    try {
        return safeStorage.decryptString(Buffer.from(token, 'base64'));
    } catch {
        return null;
    }
};

export const setSession = (session: {
    instanceUrl: string;
    siteName: string;
    token: string;
    user: AccountUser;
}) =>
    updateStore({
        ...session,
        token: safeStorage.encryptString(session.token).toString('base64'),
    });

export const clearSession = () => updateStore({ token: null, user: null });
