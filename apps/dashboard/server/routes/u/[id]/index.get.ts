const ACTIVE_CONTENT_TYPES = ['html', 'svg', 'xml', 'javascript'];

const findFile = async (idOrName: string) => {
    for (const lookup of fileLookups(idOrName)) {
        const file = await prisma.file.findFirst({
            where: lookup,
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

        if (file) return file;
    }

    return null;
};

export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;

    const fileNameOrId = decodeURIComponent(getRouterParam(event, 'id')!);
    const query = getQuery(event);
    const rangeHeader = getRequestHeader(event, 'range');

    const findFileById = await findFile(fileNameOrId);
    const access = findFileById && (await fileAccessOf(findFileById, currentUser?.id));
    const isVisitor = !(access === 'owner' || (access && !findFileById!.deletedAt));

    if (
        !findFileById ||
        (isVisitor &&
            (findFileById.deletedAt ||
                isHiddenFromVisitors(findFileById) ||
                (findFileById.maxViews && findFileById._count.views >= findFileById.maxViews)))
    ) {
        throw apiError(404, 'errorsFiles.fileNotFound');
    }

    if (isVisitor) {
        if (findFileById.folder?.public === false) {
            throw apiError(403, 'errorsFiles.noAccess');
        }

        if (findFileById.password) {
            if (!query.password) {
                return await sendRedirect(event, `/view/${fileNameOrId}`);
            }

            if (
                !(await limitPasswordAttempts(
                    event,
                    findFileById.id,
                    query.password === findFileById.password,
                ))
            ) {
                throw apiError(401, 'errorsFiles.invalidPassword');
            }
        }
    }

    let isLastView = false;

    if (isVisitor) {
        const ip = getClientIp(event);
        const userAgent = getRequestHeader(event, 'user-agent') || 'Unknown';
        const isStartOfFile = !rangeHeader || rangeHeader.startsWith('bytes=0-');

        if (isStartOfFile && claimView(`${ip}-${userAgent}-${findFileById.id}`)) {
            await prisma.view.create({
                data: {
                    fileId: findFileById.id,
                    ownerId: findFileById.authorId,
                    ip,
                },
            });

            await createLog(event, {
                action: 'View File',
                message: `Viewed ${findFileById.fileName}`,
                system: true,
            });

            sendToUser(findFileById.authorId, 'file:update', {
                id: findFileById.id,
                views: {
                    total: findFileById._count.views + 1,
                    today: (await getTodayViews([findFileById.id])).get(findFileById.id) ?? 0,
                },
            });

            isLastView =
                !!findFileById.maxViews && findFileById._count.views + 1 >= findFileById.maxViews;
        }
    }

    if (isLastView) {
        event.node.res.once('close', async () => {
            await prisma.file.delete({ where: { id: findFileById.id } }).catch(() => null);
            await removeFileData([findFileById]);

            await createLog(event, {
                action: 'Delete File',
                message: `Deleted file ${findFileById.fileName} due to max views reached`,
                system: true,
            });

            emitFilesDeleted(null, findFileById.authorId, [findFileById], 'maxViews');

            sendToUser(findFileById.authorId, 'file:delete', findFileById.id);
        });
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
            `attachment; filename="${findFileById.fileName}"; filename*=UTF-8''${encodeURIComponent(findFileById.name)}`,
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
