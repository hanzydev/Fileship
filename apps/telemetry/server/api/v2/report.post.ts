import { reportSchema } from '@fileship/telemetry';

export default defineEventHandler(async (event) => {
    assertBodySize(event);

    const body = await readValidatedBody(event, reportSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid report',
            data: { formErrors: body.error.format() },
        });
    }

    const instanceId = body.data.id.toLowerCase();

    await limitInstance(instanceId);
    await saveReport(instanceId, toStoredReport(body.data), { sentAt: body.data.sentAt });

    setResponseStatus(event, 204);
});
