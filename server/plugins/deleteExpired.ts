import { sendToUser } from './socketIO';

export default defineNitroPlugin(async () => {
    setInterval(
        async () => {
            const keys = ['file', 'code', 'url'];

            for (const key of keys) {
                const model = prisma[key as never] as any;

                const expired = (await model.findMany({
                    where: {
                        expiresAt: {
                            lte: new Date(),
                        },
                    },
                })) as any[];

                if (expired.length) {
                    await model.deleteMany({
                        where: {
                            id: {
                                in: expired.map((e) => e.id),
                            },
                        },
                    });

                    expired.forEach((e) => {
                        sendToUser(e.authorId, `delete:${key}`, e.id);
                    });
                }
            }
        },
        +(process.env.DELETE_EXPIRED_INTERVAL || 300) * 1_000,
    );
});
