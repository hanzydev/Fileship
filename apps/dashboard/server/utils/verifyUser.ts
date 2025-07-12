export const verifyUser = async (sessionId: string) => {
    if (sessionId) {
        const findSessionByPrivateId = await prisma.session.findUnique({
            where: {
                privateId: sessionId,
            },
            include: {
                user: true,
            },
        });

        if (findSessionByPrivateId) {
            return {
                ...findSessionByPrivateId.user,
                currentSessionId: findSessionByPrivateId.id,
            };
        }
    }

    return null;
};
