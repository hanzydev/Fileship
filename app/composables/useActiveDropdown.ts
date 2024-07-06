export const useActiveCtxMenu = () => {
    return useState<string | null>('activeCtxMenu', () => null);
};

export const useActiveDropdown = () => {
    return useState<string | null>('activeDropdown', () => null);
};
