export const isMobile = () => {
    const { width } = useWindowSize();
    return width.value < 768;
};
