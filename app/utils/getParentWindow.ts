export const getParentWindow = (node?: HTMLElement | null) =>
    (node && node.ownerDocument.defaultView) || self;
