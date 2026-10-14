import { reportSchema } from '@fileship/telemetry';

export default defineEventHandler(async (event) => {
    const body = reportSchema.safeParse(await readJsonBody(event));

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid report',
            data: { formErrors: body.error.format() },
        });
    }

    const instanceId = body.data.id.toLowerCase();

    await limitReports(event, instanceId);
    await saveReport(instanceId, toStoredReport(body.data), { sentAt: body.data.sentAt });

    setResponseStatus(event, 204);
});
