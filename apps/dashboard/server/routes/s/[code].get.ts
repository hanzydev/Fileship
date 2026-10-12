export default defineEventHandler(async (event) => {
    const link = await findActiveLink(getRouterParam(event, 'code')!);

    if (link.password) return sendRedirect(event, `/link/${link.code}`);

    await countLinkClick(event, link);

    setResponseHeader(event, 'Cache-Control', 'no-store');

    return sendRedirect(event, link.url);
});
