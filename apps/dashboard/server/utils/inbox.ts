import { insert } from '@orama/orama';

export const ensureInbox = async (userId: string) => {
    const existing = await prisma.inbox.findUnique({ where: { userId } });
    if (existing) return existing;

    const folder = await prisma.folder.create({
        data: {
            name: translate('errorsSweep.inboxFolder'),
            public: false,
            authorId: userId,
            inbox: {
                create: {
                    userId,
                },
            },
        },
        include: { inbox: true },
    });

    await insert(folderSearchDb, {
        id: folder.id,
        authorId: folder.authorId,
        name: folder.name,
    });

    return folder.inbox!;
};
