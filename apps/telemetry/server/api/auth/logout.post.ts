export default defineEventHandler(async (event) => {
    await endSession(event);
    return { admin: false };
});
