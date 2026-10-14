import { z } from 'zod';

const eventSchema = z.object({
    projectId: z.guid(),
    event: z.number().int(),
    payload: z.record(z.string(), z.unknown()),
    timestamp: z.number().int().positive(),
});

export default defineEventHandler(async (event) => {
    assertBodySize(event);

    const body = await readValidatedBody(event, eventSchema.safeParse);

    if (!body.success) {
        throw createError({
            status: 400,
            message: 'Invalid event',
            data: { formErrors: body.error.format() },
        });
    }

    const { projectId, event: type, payload, timestamp } = body.data;

    if (type !== 1 && type !== 2) return setResponseStatus(event, 204);

    const parse = () => {
        if (type === 1) {
            const system = legacySystemSchema.safeParse(payload);
            return system.success ? legacySystemReport(system.data) : null;
        }

        const fileship = legacyFileshipSchema.safeParse(payload);
        return fileship.success ? legacyFileshipReport(fileship.data) : null;
    };

    const report = parse();

    if (!report) {
        throw createError({
            status: 400,
            message: 'Invalid event payload',
        });
    }

    const instanceId = projectId.toLowerCase();

    await limitInstance(instanceId);
    await saveReport(instanceId, report, { sentAt: timestamp, legacy: true });

    setResponseStatus(event, 204);
});
