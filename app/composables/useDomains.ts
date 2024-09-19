export const useDomains = () => {
    return useState<string[]>('domains', () => []);
};
