declare module 'h3' {
    interface H3EventContext {
        user:
            | (Omit<User, 'limits' | 'embed'> & {
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
