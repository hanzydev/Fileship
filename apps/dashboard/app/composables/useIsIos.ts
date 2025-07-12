export const useIsIos = () => {
    return computed(
        () => /iPad|iPhone|iPod/.test(navigator?.userAgent) && !(window as any)?.MSStream,
    );
};
