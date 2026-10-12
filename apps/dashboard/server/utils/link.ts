import type { H3Event } from 'h3';
import { customAlphabet } from 'nanoid';
import { z } from 'zod';

import type { LinkModel } from '#shared/prisma/models';

const generateCode = customAlphabet('23456789abcdefghjkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ', 6);

export const linkUrlSchema = z
    .url({ protocol: /^https?$/, message: 'Enter a valid http or https URL' })
    .max(2048, 'URL must be at most 2048 characters');

export const linkCodeSchema = z
    .string()
    .trim()
    .regex(/^[\w-]{1,32}$/, 'Use up to 32 letters, numbers, dashes or underscores');

export const createLinkCode = async (): Promise<string> => {
    const code = generateCode();

    return (await prisma.link.findUnique({ where: { code } })) ? createLinkCode() : code;
};

export const assertLinkCodeFree = async (code: string) => {
    if (await prisma.link.findUnique({ where: { code } })) {
        throw createError({
            status: 409,
            message: 'That short link is already taken',
        });
    }
};

export const findActiveLink = async (code: string) => {
    const link = await prisma.link.findUnique({ where: { code } });

    if (!link || (link.expiresAt && link.expiresAt <= new Date())) {
        throw createError({
            status: 404,
            message: 'Link not found',
        });
    }

    return link;
};

export const serializeLink = (event: H3Event, link: LinkModel, domains: string[]) => ({
    ...link,
    shortUrl: buildPublicUrl(event, domains, `/s/${link.code}`),
});

export const countLinkClick = async (event: H3Event, link: LinkModel) => {
    if (link.authorId === event.context.user?.id) return;

    const updated = await prisma.link.update({
        where: { id: link.id },
        data: { clicks: { increment: 1 } },
    });

    if (updated.maxClicks && updated.clicks >= updated.maxClicks) {
        await prisma.link.deleteMany({ where: { id: link.id } });

        await createLog(null, {
            action: 'Delete Link',
            message: `Deleted link ${link.code} due to max clicks reached`,
            system: true,
        });

        sendToUser(link.authorId, 'link:delete', link.id);
        return;
    }

    sendToUser(link.authorId, 'link:update', { id: link.id, clicks: updated.clicks });
};
