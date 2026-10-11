import type { SessionModel } from '#shared/prisma/models';

export type SessionData = Omit<SessionModel, 'privateId' | 'userId' | 'lastVerify'>;
