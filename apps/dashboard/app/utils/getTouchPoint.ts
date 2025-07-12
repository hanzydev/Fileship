export const getTouchPoint = (touches: TouchList, touchId: null | number): Touch => {
    for (let i = 0; i < touches.length; i++) {
        if (touches[i]!.identifier === touchId) {
            return touches[i]!;
        }
    }

    return touches[0]!;
};
