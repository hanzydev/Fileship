import { z } from 'zod';

const validationSchema = settingsSchema
    .pick({ smtpPort: true, smtpSecurity: true, smtpUser: true, smtpPassword: true })
    .extend({
        smtpHost: z.string().trim().min(1, 'instance.email.hostRequired').max(256),
        smtpFrom: z.email().max(256),
        to: z.email().max(256),
        verificationData: z.any().optional(),
    });

export default defineEventHandler(async (event) => {
    superAdminOnly(event);

    const body = await readValidatedBody(event, validationSchema.safeParse);
    if (!body.success) throw invalidBodyError(body.error);

    const { to, verificationData, ...smtp } = body.data;
    const settings = await getSettings();

    if (!smtp.smtpPassword && settings.smtpPassword) await verifySession(event, verificationData);

    await rateLimit(`emailTest:${event.context.user!.id}`, 10, 10 * 60 * 1000);

    try {
        await sendEmail({
            to,
            language: requestLanguage(event),
            template: 'test',
            settings: {
                ...settings,
                ...smtp,
                smtpPassword: smtp.smtpPassword || settings.smtpPassword,
            },
        });
    } catch (error) {
        throw apiError(502, 'errorsEmail.testFailed', { error: (error as Error).message });
    }

    await createLog(event, {
        action: 'Send Test Email',
        message: `Sent a test email to ${to}`,
    });
});
