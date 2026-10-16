import { z } from 'zod';

const HOST =
    /^(?:(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\.)*[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?|\[[0-9a-f:.]+\])(?::(?:[1-9]\d{0,3}|[1-5]\d{4}|6[0-4]\d{3}|65[0-4]\d{2}|655[0-2]\d|6553[0-5]))?$/i;

const validationSchema = z
    .array(z.string().max(253, 'errorsUsers.domainTooLong'))
    .max(50, 'errorsUsers.tooManyDomains')
    .transform((value) => [
        ...new Set(value.map((domain) => domain.trim()).filter((domain) => domain.length)),
    ]);

export default defineEventHandler(async (event) => {
    userOnly(event);

    const currentUser = event.context.user!;
    const body = await readValidatedBody(event, validationSchema.safeParse);

    if (!body.success) {
        throw invalidBodyError(body.error);
    }

    const invalid = body.data.find((domain) => !HOST.test(domain));

    if (invalid) {
        throw apiError(400, 'errorsUsers.invalidDomain', { domain: invalid });
    }

    await prisma.user.update({
        where: {
            id: currentUser.id,
        },
        data: {
            domains: body.data,
        },
    });

    sendToUser(currentUser.id, 'currentUser:domainsUpdate', body.data);

    return body.data;
});
