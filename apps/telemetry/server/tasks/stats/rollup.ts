import consola from 'consola';
import dayjs from 'dayjs';

export default defineTask({
    meta: {
        name: 'stats:rollup',
        description: 'Computes the daily statistics that are missing, plus today and yesterday.',
    },
    async run() {
        const days = await rollup();

        if (days > 2) {
            consola.success(
                `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Computed statistics for ${days} days.`,
            );
        }

        return { result: days };
    },
});
