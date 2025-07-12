export const getTouchId = (event: MouseEvent | TouchEvent): number | null =>
    isTouch(event) ? event.changedTouches[0]!.identifier : null;
