export const isTouch = (event: MouseEvent | TouchEvent): event is TouchEvent =>
    'touches' in event;
