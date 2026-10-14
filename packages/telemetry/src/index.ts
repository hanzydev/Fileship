import os from 'node:os';
import { resolve } from 'node:path';

import checkDiskSpace from 'check-disk-space';
import isDocker from 'is-docker';
import { ofetch } from 'ofetch';

import type { TelemetryReport, TelemetryRuntime } from './schema';

export * from './schema';

export const TELEMETRY_URL = 'https://fileship-telemetry.hanzy.dev';

export const collectRuntime = async (directory: string): Promise<TelemetryRuntime> => {
    const cpus = os.cpus();
    const disk = await checkDiskSpace(resolve(directory)).catch(() => ({ size: 0, free: 0 }));

    return {
        node: process.version,
        platform: os.platform(),
        release: os.release().slice(0, 128),
        arch: os.arch(),
        cpuCount: cpus.length,
        cpuModel: (cpus[0]?.model ?? '').replace(/\s+/g, ' ').trim().slice(0, 128),
        memoryTotal: os.totalmem(),
        diskTotal: disk.size,
        diskFree: disk.free,
        docker: isDocker(),
        uptime: Math.round(process.uptime()),
    };
};

export const sendReport = (report: TelemetryReport, url = TELEMETRY_URL) =>
    ofetch(`${url.replace(/\/+$/, '')}/api/v2/report`, {
        method: 'POST',
        body: report,
        timeout: 10_000,
        retry: 2,
        retryDelay: 5_000,
    });
