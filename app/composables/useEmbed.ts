export const useEmbed = () => {
    return useState<IEmbed>('embed', () => defaultEmbed);
};
