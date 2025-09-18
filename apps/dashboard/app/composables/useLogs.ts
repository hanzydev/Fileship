import type { LogModel } from '#shared/prisma/models';

export type LogData = LogModel;

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
