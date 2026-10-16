export const useAdmin = () => {
    return useState<boolean>('admin', () => false);
};
