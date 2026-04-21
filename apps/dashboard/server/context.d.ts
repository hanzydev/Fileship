import type { UserModel } from '#shared/prisma/models';

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
