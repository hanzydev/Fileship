export default defineEventHandler(async (event) => {
    const currentUser = event.context.user;
    const folderId = getRouterParam(event, 'id');
    const jobId = getRouterParam(event, 'jobId');
    const storage = useStorage('cache');

    const archiveStatus = await storage.getItem<{
        isPrivate: boolean;
        isDone: boolean;
        authorId: string;
    }>(`folderArchiveStatus:${folderId}:${jobId}`);

    if (!archiveStatus) {
        throw createError({
            statusCode: 404,
            message: 'Job not found',
        });
    }

    if (archiveStatus.isPrivate && archiveStatus.authorId !== currentUser?.id) throw forbiddenError;

    return { isDone: archiveStatus.isDone };
});
