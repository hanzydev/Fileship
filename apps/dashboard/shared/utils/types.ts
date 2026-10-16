export interface IEmbed {
    enabled: boolean;
    color: string;
    title: string;
    description: string;
    siteName: string;
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
