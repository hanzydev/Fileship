import type { User } from '@prisma/client';

import type { IEmbed, IUserLimits } from '~~/utils/types';

declare module 'h3' {
    interface H3EventContext {
        user:
            | (User & {
                  currentSessionId: string;
                  limits: IUserLimits;
                  embed: IEmbed;
              })
            | null;
        sessionPrivateId: string | null;
    }
}

export {};
