export const useActiveCtxMenu = () => {
    return useState<string | null>('activeCtxMenu', () => null);
};
