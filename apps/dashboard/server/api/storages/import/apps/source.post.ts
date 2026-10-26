export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    return { id: await saveImportSource(event) };
});
