import { defu } from 'defu';

export default defineEventHandler(async (event) => {
    userOnly(event);
    return defu(event.context.user!.embed, defaultEmbed) as IEmbed;
});
