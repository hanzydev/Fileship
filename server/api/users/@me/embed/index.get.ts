import { defu } from 'defu';

import { defaultEmbed } from '~~/utils/constants';
import type { IEmbed } from '~~/utils/types';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    if (!currentUser) {
        throw createError({
            statusCode: 401,
            statusMessage: 'Unauthorized',
            message: 'You do not have permission to perform this action',
        });
    }

    return defu(currentUser.embed, defaultEmbed) as IEmbed;
});
