import type { Log } from '@prisma/client';

export type LogData = Omit<Log, 'id'>;

export type LogUser = {
    id: string;
    username: string;
    avatar: string | null;
};

export const useLogs = () => {
    return useState<{
        logs: LogData[];
        users: LogUser[];
    }>('logs', () => ({
        logs: [],
        users: [],
    }));
};
