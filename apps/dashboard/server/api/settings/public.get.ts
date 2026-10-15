export default defineEventHandler(async () => ({
    ...(await getPublicSettings()),
    uploadConcurrency: (await getStorage(await getActiveStorageId())).isSequential ? 1 : 4,
}));
