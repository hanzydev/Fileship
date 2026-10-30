import type { DiscordLayout } from './discordEmbed';

export interface IEmbed {
    enabled: boolean;
    color: string;
    title: string;
    description: string;
    siteName: string;
    discord: {
        enabled: boolean;
        file: DiscordLayout | null;
        note: DiscordLayout | null;
        folder: DiscordLayout | null;
    };
}

export interface IMediaMetadata {
    width?: number;
    height?: number;
    duration?: number;
    bitrate?: number;
    fps?: number;
    format?: string;
    videoCodec?: string;
    audioCodec?: string;
    sampleRate?: number;
    channels?: number;
    colorSpace?: string;
    bitDepth?: number;
    hasAlpha?: boolean;
    frames?: number;
    pages?: number;
    thumbnailFailed?: boolean;
}

export interface IUserLimits {
    usableSpace: number;
    backupLimit: number;
}

export interface IUserAiSettings {
    enabled: boolean;
    suppressPii: boolean;
}

export type FolderAccess = 'owner' | 'editor' | 'viewer';
