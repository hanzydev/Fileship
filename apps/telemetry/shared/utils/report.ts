import { z } from 'zod';

import type { TelemetryReport } from '@fileship/telemetry';

type DeepPartial<T> = { [K in keyof T]?: T[K] extends object ? DeepPartial<T[K]> : T[K] };

export type StoredReport = DeepPartial<Omit<TelemetryReport, 'id' | 'sentAt'>>;

const count = z.coerce.number().int().min(0);
const bytes = z.coerce.number().min(0);
const text = (max: number) => z.string().trim().max(max);

export const legacySystemSchema = z.object({
    cpu: z.object({ count, model: text(128) }),
    memory: z.object({ total: bytes }),
    disk: z.object({ total: bytes, free: bytes }),
    os: z.object({ platform: text(32), release: text(128), arch: text(32) }),
    isDocker: z.boolean(),
});

export const legacyFileshipSchema = z.object({
    version: text(32),
    files: count,
    folders: count,
    notes: count,
    users: count,
    nodeVersion: text(32).optional(),
});

export const legacySystemReport = (payload: z.infer<typeof legacySystemSchema>): StoredReport => ({
    runtime: {
        cpuCount: payload.cpu.count,
        cpuModel: payload.cpu.model.replace(/\s+/g, ' ').trim(),
        memoryTotal: payload.memory.total,
        diskTotal: payload.disk.total,
        diskFree: payload.disk.free,
        platform: payload.os.platform,
        release: payload.os.release,
        arch: payload.os.arch,
        docker: payload.isDocker,
    },
});

export const legacyFileshipReport = (
    payload: z.infer<typeof legacyFileshipSchema>,
): StoredReport => ({
    version: payload.version,
    runtime: payload.nodeVersion ? { node: payload.nodeVersion } : {},
    usage: {
        users: { total: payload.users },
        files: { total: payload.files },
        folders: { total: payload.folders },
        notes: { total: payload.notes },
    },
});

export const toStoredReport = ({ id: _id, sentAt: _sentAt, ...report }: TelemetryReport) =>
    report as StoredReport;

export const startOfDay = (date: Date) =>
    new Date(Date.UTC(date.getUTCFullYear(), date.getUTCMonth(), date.getUTCDate()));

export const addDays = (date: Date, days: number) => new Date(date.getTime() + days * 86_400_000);

export const toDateString = (date: Date) => date.toISOString().slice(0, 10);
