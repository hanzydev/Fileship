export default defineEventHandler((event) => ({ admin: !!event.context.admin }));
