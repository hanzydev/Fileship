import { filesize } from 'filesize';

import type { Prisma } from '~~/generated/prisma/client';

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');

    const findFolderById = await prisma.folder.findUnique({
        where: {
            id: folderId,
            deletedAt: null,
        },
        include: {
            author: {
                select: {
                    domains: true,
                    aiSettings: true,
                },
            },
        },
    });

    if (!findFolderById) {
        throw createError({
            status: 404,
            message: 'Folder not found',
        });
    }

    const isOwner = findFolderById.authorId === currentUser?.id;

    if (!isOwner && !findFolderById.public) throw forbiddenError;

    const query = await readListQuery(event, {
        mode: fileListShape.mode,
        type: fileListShape.type,
    });
    const lockedIds =
        query.q && !isOwner
            ? (
                  await prisma.file.findMany({
                      where: {
                          folderId: findFolderById.id,
                          deletedAt: null,
                          password: { not: null },
                          OR: [
                              { fileName: { contains: query.q, mode: 'insensitive' } },
                              { name: { contains: query.q, mode: 'insensitive' } },
                          ],
                      },
                      select: { id: true },
                  })
              ).map((file) => file.id)
            : [];

    const searchIds = query.q
        ? [
              ...(await searchFileIds(
                  query.q,
                  findFolderById.authorId,
                  isOwner ? query.mode : 'fulltext',
                  new Set(
                      (
                          await prisma.file.findMany({
                              where: { folderId: findFolderById.id, deletedAt: null },
                              select: { id: true },
                          })
                      ).map((file) => file.id),
                  ),
              )),
              ...lockedIds,
          ]
        : undefined;

    const where: Prisma.FileWhereInput = {
        folderId: findFolderById.id,
        deletedAt: null,
        ...(isOwner ? {} : await visitorFileWhere(findFolderById.author)),
        AND: [
            query.type.length ? { OR: query.type.map(fileCategoryWhere) } : {},
            searchIds ? { id: { in: searchIds } } : {},
            searchIds && !isOwner ? { OR: [{ password: null }, { id: { in: lockedIds } }] } : {},
        ],
    };

    const args = {
        omit: {
            embedding: true,
            textEmbedding: true,
            ocrText: true,
            maxViews: true,
            expiresAt: true,
            folderId: true,
            ...(isOwner
                ? {}
                : { caption: true, piiReasons: true, piiOverride: true, authorId: true }),
        },
        include: {
            _count: { select: { views: true } },
        },
    } satisfies Prisma.FileFindManyArgs;

    const relevance = await findRelevancePage(query, searchIds, () =>
        prisma.file.findMany({ where, select: { id: true } }),
    );

    const [items, total] = relevance
        ? [
              sortByIds(
                  await prisma.file.findMany({ ...args, where: { id: { in: relevance.ids } } }),
                  relevance.ids,
              ),
              relevance.total,
          ]
        : await Promise.all([
              prisma.file.findMany({
                  ...args,
                  where,
                  orderBy: fileOrderBy(query.sort),
                  skip: query.offset,
                  take: query.limit,
              }),
              prisma.file.count({ where }),
          ]);

    const todayViews = isOwner ? await getTodayViews(items.map((file) => file.id)) : new Map();

    return {
        items: items.map(({ password, ...file }) =>
            password && !isOwner
                ? {
                      id: file.id,
                      fileName: file.fileName,
                      name: file.name,
                      mimeType: file.mimeType,
                      size: { raw: file.size.toString(), formatted: filesize(file.size) },
                      createdAt: file.createdAt,
                      isLocked: true,
                  }
                : {
                      ...serializeFile(
                          event,
                          file,
                          findFolderById.author.domains,
                          todayViews.get(file.id),
                      ),
                      isLocked: false,
                  },
        ),
        total,
    };
});
