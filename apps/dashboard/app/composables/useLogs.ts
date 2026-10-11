import type { LogModel } from '#shared/prisma/models';

export type LogUser = {
    id: string;
    username: string;
    avatar: string | null;
};

export type LogData = LogModel & { user: LogUser | null };
