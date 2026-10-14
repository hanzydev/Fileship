import consola from 'consola';
import dayjs from 'dayjs';

const REPORT_RETENTION_MONTHS = 13;

export default defineTask({
    meta: {
        name: 'db:prune',
        description: 'Deletes instances that stopped reporting, old reports and expired sessions.',
    },
    async run() {
        await rollup();

        const { instanceRetentionDays } = useRuntimeConfig();

        const [instances, reports] = await prisma.$transaction([
            prisma.instance.deleteMany({
                where: {
                    lastSeenAt: { lt: dayjs().subtract(instanceRetentionDays, 'day').toDate() },
                },
            }),
            prisma.report.deleteMany({
                where: {
                    day: { lt: dayjs().subtract(REPORT_RETENTION_MONTHS, 'month').toDate() },
                },
            }),
            prisma.session.deleteMany({ where: { expiresAt: { lt: new Date() } } }),
        ]);

        if (instances.count || reports.count) {
            consola.success(
                `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Deleted ${instances.count} instances and ${reports.count} reports.`,
            );
        }

        return { result: { instances: instances.count, reports: reports.count } };
    },
});
