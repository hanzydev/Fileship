import { ofetch } from 'ofetch';

export const updaterUrl = process.env.UPDATER_URL || 'http://updater-agent:3001';
export const updaterSecret = process.env.UPDATER_SECRET || 'dev-secret-token';

export const updaterFetch = ofetch.create({
    baseURL: updaterUrl,
    headers: {
        Authorization: updaterSecret,
    },
    timeout: 5_000,
});
