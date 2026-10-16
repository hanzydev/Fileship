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
        throw apiError(404, 'errorsContent.jobNotFound');
    }

    if (archiveStatus.isPrivate && archiveStatus.authorId !== currentUser?.id)
        throw forbiddenError();

    return { isDone: archiveStatus.isDone };
});
