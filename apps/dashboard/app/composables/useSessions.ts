import type { Session } from '@prisma/client';

export type SessionData = Omit<Session, 'privateId' | 'userId' | 'lastVerify'>;

export const useSessions = () => {
    return useState<SessionData[]>('sessions', () => []);
};
