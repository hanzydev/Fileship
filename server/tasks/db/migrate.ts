import consola from 'consola';
import dayjs from 'dayjs';
import { join } from 'pathe';

import __internals from '@prisma/internals';
import __migrate from '@prisma/migrate';

const { loadSchemaContext } = __internals;
const { Migrate } = __migrate;

export default defineTask({
    meta: {
        name: 'db:migrate',
        description: 'Migrates database',
    },
    async run() {
        const schemaContext = await loadSchemaContext({
            schemaPathFromArg: join(baseDirectory, 'prisma', 'schema.prisma'),
        });

        const migrate = await Migrate.setup({
            schemaContext,
            migrationsDirPath: join(baseDirectory, 'prisma', 'migrations'),
        });

        try {
            const { appliedMigrationNames } = await migrate.applyMigrations();

            if (appliedMigrationNames.length) {
                consola.success(
                    `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Applied migrations: ${appliedMigrationNames.join(', ')}`,
                );
            } else {
                consola.info(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - No migrations to apply.`);
            }

            return { result: 'success' };
        } catch (error) {
            consola.error(
                `${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Failed to apply migrations. ${error}`,
            );

            return { result: 'failed' };
        } finally {
            migrate.stop();
        }
    },
});
