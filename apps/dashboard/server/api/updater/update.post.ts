import { z } from 'zod';

const validationSchema = z.object({ verificationData: z.any().optional() }).optional();

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    await verifySession(event, body.data?.verificationData);

    const isRestoring = await prisma.user.count({
        where: { backupRestoreState: { not: null } },
    });

    if (isRestoring) {
        throw apiError(409, 'errorsSystem.restoreInProgress');
    }

    await createLog(event, {
        action: 'Update Fileship',
        message: 'Started an update',
    });

    return sendProxy(event, `${updaterUrl}/api/update`, {
        fetchOptions: {
            method: 'POST',
            headers: {
                Authorization: updaterSecret,
            },
        },
    });
});
