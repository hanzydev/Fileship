import type { UserModel } from '#shared/prisma/models';
import type { IEmbed, IUserAiSettings, IUserLimits } from '#shared/utils/types';

declare module 'h3' {
    interface H3EventContext {
        user:
            | (Omit<UserModel, 'limits' | 'embed'> & {
                  currentSessionId: string;
                  limits: IUserLimits;
                  embed: IEmbed;
                  aiSettings: IUserAiSettings;
              })
            | null;
        sessionPrivateId: string | null;
    }
}

export {};
