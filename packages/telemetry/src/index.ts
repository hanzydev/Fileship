import os from 'node:os';

import checkDiskSpace from 'check-disk-space';
import isDocker from 'is-docker';
import { ofetch } from 'ofetch';

export enum TelemetryEventType {
    SystemInformation = 1,
    FileshipInformation = 2,
    AISearchUsage = 3,
}

export interface TelemetryServiceOptions {
    enabled: boolean;
    projectId: string;
}

export interface TelemetryEvent {
    event: TelemetryEventType;
    payload: FileshipInfoPayload | SystemInfoPayload | AISearchUsagePayload;
}

export interface FileshipInfoPayload {
    version: string;
    files: number;
    codes: number;
    folders: number;
    notes: number;
    urls: number;
    users: number;
    nodeVersion?: string;
}

export interface SystemInfoPayload {
    cpu: {
        count: number;
        model: string;
    };
    memory: {
        total: number;
        free: number;
        used: number;
    };
    disk: {
        total: number;
        free: number;
        used: number;
    };
    os: {
        platform: string;
        release: string;
        arch: string;
    };
    isDocker: boolean;
}

export interface AISearchUsagePayload {
    query: string;
    results: number;
    duration: number;
}

export class Telemetry {
    public enabled: boolean;
    public projectId: string;

    private apiFetch = ofetch.create({
        baseURL: 'https://fileship-telemetry.hanzy.dev/api',
    });

    public constructor({ enabled, projectId }: TelemetryServiceOptions) {
        this.enabled = enabled;
        this.projectId = projectId;
    }

    private async invokeEvent(event: TelemetryEvent) {
        if (!this.enabled) return;

        try {
            await this.apiFetch('/event', {
                method: 'POST',
                body: {
                    ...event,
                    timestamp: Date.now(),
                    projectId: this.projectId,
                },
            });
        } catch {
            //
        }
    }

    public collectFileshipInfo(payload: FileshipInfoPayload) {
        return this.invokeEvent({
            event: TelemetryEventType.FileshipInformation,
            payload: {
                ...payload,
                nodeVersion: process.version,
            },
        });
    }

    public async collectSystemInfo() {
        const directoryPath = os.platform() === 'win32' ? 'C:\\' : '/';
        const diskSpace = await checkDiskSpace(directoryPath).catch(() => ({
            size: 0,
            free: 0,
        }));

        return this.invokeEvent({
            event: TelemetryEventType.SystemInformation,
            payload: {
                cpu: {
                    count: os.cpus().length,
                    model: os.cpus()[0].model,
                },
                memory: {
                    total: os.totalmem(),
                    free: os.freemem(),
                    used: os.totalmem() - os.freemem(),
                },
                disk: {
                    total: diskSpace.size,
                    free: diskSpace.free,
                    used: diskSpace.size - diskSpace.free,
                },
                os: {
                    platform: os.platform(),
                    release: os.release(),
                    arch: os.arch(),
                },
                isDocker: isDocker(),
            },
        });
    }

    public collectAISearchUsage(payload: AISearchUsagePayload) {
        return this.invokeEvent({
            event: TelemetryEventType.AISearchUsage,
            payload,
        });
    }
}
