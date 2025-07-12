export type PasskeyData = {
    id: string;
    name: string;
    createdAt: Date;
};

export const usePasskeys = () => {
    return useState<PasskeyData[]>('passkeys', () => []);
};
