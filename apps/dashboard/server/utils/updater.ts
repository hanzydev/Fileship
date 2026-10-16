import { ofetch } from 'ofetch';

export const updaterUrl = useRuntimeConfig().updater.url;
export const updaterSecret = useRuntimeConfig().updater.secret;

export const updaterFetch = ofetch.create({
    baseURL: updaterUrl,
    headers: {
        Authorization: updaterSecret,
    },
    timeout: 5_000,
});
