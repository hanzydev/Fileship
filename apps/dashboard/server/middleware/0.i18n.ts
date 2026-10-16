export default defineEventHandler(async (event) => {
    event.context.t = await useTranslation(event);
});
