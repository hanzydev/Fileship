import type { SessionModel } from '#shared/prisma/models';

export type SessionData = Pick<
    SessionModel,
    'id' | 'ip' | 'os' | 'platform' | 'location' | 'lastSeen'
>;
