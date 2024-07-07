import { hash } from 'argon2';

export default defineNitroPlugin(async () => {
    const findUser = await prisma.user.findFirst();

    if (!findUser) {
        await prisma.user.create({
            data: {
                username: 'root',
                password: await hash('password'),
            },
        });
    }
});
