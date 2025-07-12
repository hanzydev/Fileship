export interface IEmbed {
    enabled: boolean;
    color: string;
    title: string;
    description: string;
    siteName: string;
}

export interface IUserLimits {
    usableSpace: number;
    backupLimit: number;
}
