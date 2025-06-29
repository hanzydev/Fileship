import { hash } from 'argon2';
import consola from 'consola';
import dayjs from 'dayjs';

import { UserPermission } from '@prisma/client';

import { defaultUserLimits } from '~~/shared/utils/constants';

export default defineTask({
    meta: {
        name: 'db:createDefaultUser',
        description: 'Creates default user',
    },
    async run() {
        const findUser = await prisma.user.findFirst();
        if (!findUser) {
            await prisma.user.create({
                data: {
                    username: 'admin',
                    password: await hash('password'),
                    superAdmin: true,
                    permissions: [UserPermission.Admin],
                    limits: defaultUserLimits as never,
                },
            });

            consola.success(`${dayjs().format('YYYY-MM-DD HH:mm:ss')} - Default user created`);
        }

        return { result: 'success' };
    },
});
