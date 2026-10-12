const VIEW_WINDOW = 10 * 60 * 1_000;

const ACTIVE_CONTENT_TYPES = ['html', 'svg', 'xml', 'javascript'];

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const fileNameOrId = decodeURIComponent(getRouterParam(event, 'id')!);
    const query = getQuery(event);
    const rangeHeader = getRequestHeader(event, 'range');
    const storage = useStorage('cache');

    const findFileById = await prisma.file.findFirst({
        where: {
            OR: [
                {
                    id: fileNameOrId,
                },
                {
                    fileName: fileNameOrId,
                },
            ],
        },
        include: {
            _count: {
                select: {
                    views: true,
                },
            },
            folder: {
                select: {
                    public: true,
                },
            },
        },
    });

    if (!findFileById) {
        throw createError({
            status: 404,
            message: 'File not found',
        });
    }

    if (findFileById.authorId !== currentUser?.id) {
        if (findFileById.folder?.public === false) {
            throw createError({
                status: 403,
                message: 'You do not have permission to access this page',
            });
        }

        if (findFileById.password) {
            if (!query.password) {
                return await sendRedirect(event, `/view/${fileNameOrId}`);
            }

            if (query.password !== findFileById.password) {
                throw createError({
                    status: 401,
                    message: 'Invalid password',
                });
            }
        }
    }

    if (findFileById.authorId !== currentUser?.id) {
        const ip = getRequestIP(event, { xForwardedFor: true }) || 'Unknown';
        const userAgent = getRequestHeader(event, 'user-agent') || 'Unknown';
        const key = `${ip}-${userAgent}-${findFileById.id}`;
        const now = Date.now();
        const lastView = (await storage.getItem<number>(key)) || 0;

        const isStartOfFile = !rangeHeader || rangeHeader.startsWith('bytes=0-');
        const shouldCountView = isStartOfFile && now - lastView > VIEW_WINDOW;

        if (shouldCountView) {
            await prisma.view.create({
                data: {
                    fileId: findFileById.id,
                    ownerId: findFileById.authorId,
                    ip: getRequestIP(event, { xForwardedFor: true }) ?? 'Unknown',
                },
            });

            await createLog(event, {
                action: 'View File',
                message: `Viewed ${findFileById.fileName}`,
                system: true,
            });

            const currentViews = findFileById._count.views + 1;

            if (findFileById.maxViews && currentViews >= findFileById.maxViews) {
                await prisma.file.delete({
                    where: {
                        id: findFileById.id,
                    },
                });

                await removeFileData([findFileById]);

                await createLog(event, {
                    action: 'Delete File',
                    message: `Deleted file ${findFileById.fileName} due to max views reached`,
                    system: true,
                });

                sendToUser(findFileById.authorId, 'file:delete', findFileById.id);

                throw createError({
                    status: 404,
                    message: 'File not found',
                });
            }
        }
    }

    setResponseHeaders(event, {
        'Content-Type': findFileById.mimeType,
        'Accept-Ranges': 'bytes',
        'X-Content-Type-Options': 'nosniff',
        'Content-Security-Policy': 'sandbox',
    });

    if (
        'download' in query ||
        ACTIVE_CONTENT_TYPES.some((type) => findFileById.mimeType.includes(type))
    ) {
        setResponseHeader(
            event,
            'Content-Disposition',
            `attachment; filename="${findFileById.fileName}"`,
        );
    }

    const fileSize = Number(findFileById.size);
    const fileStorage = await getStorage(findFileById.storageId);
    const key = uploadKey(findFileById.fileName);

    if (rangeHeader) {
        let start;
        let end = fileSize - 1;

        const rangeString = rangeHeader.trim().slice(6);

        if (rangeString.startsWith('-')) {
            const suffixLength = +rangeString.slice(1);
            start = fileSize - suffixLength;

            if (start < 0) start = 0;
        } else {
            const [_start, _end] = rangeString.split('-');
            start = +_start!;

            if (_end) end = +_end;
        }

        if (!isNaN(start) && !isNaN(end) && start < fileSize && start <= end) {
            if (end >= fileSize) end = fileSize - 1;

            const stream = await fileStorage.read(key, { start, end });

            setResponseStatus(event, 206);
            setResponseHeader(event, 'Content-Range', `bytes ${start}-${end}/${fileSize}`);
            setResponseHeader(event, 'Content-Length', end - start + 1);

            return sendStream(event, stream);
        }
    }

    const stream = await fileStorage.read(key);

    setResponseHeader(event, 'Content-Length', fileSize);

    return sendStream(event, stream);
});
