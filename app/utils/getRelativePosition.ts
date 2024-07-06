export const getRelativePosition = (
    node: HTMLElement,
    event: MouseEvent | TouchEvent,
    touchId: null | number,
): {
    left: number;
    top: number;
} => {
    const rect = node.getBoundingClientRect();

    const pointer = isTouch(event)
        ? getTouchPoint(event.touches, touchId)
        : (event as MouseEvent);

    const parentWindow = getParentWindow(node);

    return {
        top: clamp(
            (pointer.pageY - (rect.top + parentWindow.scrollY)) / rect.height,
            0,
            1,
        ),
        left: clamp(
            (pointer.pageX - (rect.left + parentWindow.scrollX)) / rect.width,
            0,
            1,
        ),
    };
};
