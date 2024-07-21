import { hash } from 'argon2';

import { UserPermission } from '@prisma/client';

import { defaultUserLimits } from '~~/utils/constants';

export default defineNitroPlugin(async () => {
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
    }
});
