import type { UserModel } from '#shared/prisma/models';

declare module 'h3' {
    interface H3EventContext {
        user:
            | (Omit<UserModel, 'limits' | 'embed'> & {
                  currentSessionId: string;
                  impersonator: { sessionId: string; username: string } | null;
                  limits: IUserLimits;
                  embed: IEmbed;
                  aiSettings: IUserAiSettings;
              })
            | null;
    }
}

export {};
