import dayjs from 'dayjs';

import { AIJobStatus } from '#shared/prisma/enums';

export default defineTask({
    meta: {
        name: 'ai:cleanup',
        description: 'Removes failed AI jobs older than 30 days',
    },
    async run() {
        const { count } = await prisma.aIJob.deleteMany({
            where: {
                status: AIJobStatus.Failed,
                updatedAt: { lt: dayjs().subtract(30, 'day').toDate() },
            },
        });

        return { result: 'success', deleted: count };
    },
});
