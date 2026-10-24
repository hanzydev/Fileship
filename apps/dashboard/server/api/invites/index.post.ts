import { randomBytes } from 'node:crypto';

import { z } from 'zod';

import { UserPermission } from '#shared/prisma/enums';

const validationSchema = z.object({
    email: emailAddressSchema,
    permissions: z.array(z.enum(UserPermission)).min(1, 'errorsUsers.permissionsRequired'),
    usableSpace: z.number().int('errorsUsers.usableSpaceInt').min(-1, 'errorsUsers.usableSpaceMin'),
    language: z.enum(LANGUAGES),
    verificationData: z.any().optional(),
});

export default defineEventHandler(async (event) => {
    adminOnly(event);

    if (!(await isEmailEnabled())) throw apiError(400, 'errorsEmail.disabled');

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const currentUser = event.context.user!;
    const { email, usableSpace, language } = body.data;
    const permissions = body.data.permissions.includes(UserPermission.Admin)
        ? [UserPermission.Admin]
        : body.data.permissions;

    if (permissions.includes(UserPermission.Admin)) {
        await verifySession(event, body.data.verificationData);
    }

    if (await prisma.user.count({ where: { email } })) {
        throw apiError(409, 'errorsEmail.emailTaken', undefined, {
            formErrors: { email: { _errors: [translate('errorsEmail.emailTaken')] } },
        });
    }

    await rateLimit(`inviteCreate:${currentUser.id}`, 20, 60 * 60 * 1000);

    const token = randomBytes(32).toString('base64url');

    const [, invite] = await prisma.$transaction([
        prisma.invite.deleteMany({ where: { email } }),
        prisma.invite.create({
            data: {
                tokenHash: hashToken(token),
                email,
                permissions,
                usableSpace,
                invitedById: currentUser.id,
                expiresAt: new Date(Date.now() + 7 * 24 * 60 * 60 * 1000),
            },
            select: inviteSelect,
        }),
    ]);

    try {
        await sendEmail({
            to: email,
            language,
            template: 'invite',
            params: { username: currentUser.username },
            path: `/register?invite=${token}`,
        });
    } catch {
        await prisma.invite.delete({ where: { id: invite.id } });
        throw apiError(502, 'errorsEmail.sendFailed');
    }

    await createLog(event, {
        action: 'Create Invite',
        message: `Invited ${email} with permissions ${permissions.join(', ')}`,
    });

    await sendByFilter(isAdmin, 'invite:update', null);

    return invite;
});
