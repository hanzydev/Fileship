export const verifyUser = async (sessionId: string) => {
    if (sessionId) {
        const findSessionByPrivateId = await prisma.session.findUnique({
            where: {
                privateId: sessionId,
            },
        });

        if (findSessionByPrivateId) {
            const findUserById = await prisma.user.findUnique({
                where: {
                    id: findSessionByPrivateId.userId,
                },
            });

            if (findUserById) {
                return {
                    ...findUserById,
                    currentSessionId: findSessionByPrivateId.id,
                };
            }
        }
    }

    return null;
};
