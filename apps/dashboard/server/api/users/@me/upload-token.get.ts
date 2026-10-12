export default defineEventHandler(async (event) => {
    fileUploaderOnly(event);

    const currentUser = event.context.user!;

    return {
        token: currentUser.uploadTokenEncrypted
            ? await decrypt(currentUser.uploadTokenEncrypted)
            : await generateUploadToken(currentUser.id),
    };
});
