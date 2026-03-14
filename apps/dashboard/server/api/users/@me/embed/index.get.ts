import { defu } from 'defu';

import { defaultEmbed } from '#shared/utils/constants';
import type { IEmbed } from '#shared/utils/types';

export default defineEventHandler(async (event) => {
    userOnly(event);
    return defu(event.context.user!.embed, defaultEmbed) as IEmbed;
});
