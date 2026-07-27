export default defineEventHandler(async (event) => {
    adminOnly(event);

    return sendProxy(event, `${updaterUrl}/api/update`, {
        fetchOptions: {
            method: 'POST',
            headers: {
                Authorization: updaterSecret,
            },
        },
    });
});
