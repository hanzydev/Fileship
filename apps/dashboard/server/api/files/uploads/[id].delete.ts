export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const uploadId = getRouterParam(event, 'id')!;
    if (!/^[\w-]{1,64}$/.test(uploadId)) throw apiError(400, 'errorsFiles.invalidUploadId');

    await cancelUploadSession(event.context.user!.id, uploadId);
});
