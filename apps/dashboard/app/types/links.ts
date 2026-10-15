import type { LinkModel } from '#shared/prisma/models';

export type LinkData = LinkModel & { shortUrl: string };
