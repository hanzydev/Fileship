import type { Log } from '@prisma/client';

export type LogData = Omit<Log, 'id' | 'userId'> & {
    user: { id: string; username: string } | null;
};

export const useLogs = () => {
    return useState<LogData[]>('logs', () => []);
};
