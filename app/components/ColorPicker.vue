<template>
    <UiDropdown v-model="isOpen" placement="top">
        <slot />
        <template #content>
            <div
                relative
                top-6
                z10
                w200px
                rounded
                bg-fs3
                p2
                ring-2
                :style="{
                    '--un-ring-color': _color,
                }"
            >
                <div
                    ref="saturationRef"
                    relative
                    h164px
                    wfull
                    rounded-t-0.75
                    bg="[linear-gradient(0deg,#000,transparent),linear-gradient(90deg,#fff,hsla(0,0%,100%,0))]"
                    :style="{
                        backgroundColor: hueColor,
                    }"
                    @mousedown="handleSaturationMouseDown"
                    @touchstart="handleSaturationMouseDown"
                >
                    <div
                        ref="saturationPointerRef"
                        absolute
                        right-0
                        top-0
                        z10
                        rounded-full
                        translate-x-="1/2"
                        translate-y-="1/2"
                        ring="2 white"
                        :style="{
                            backgroundColor: `rgb(${selectedColor.r}, ${selectedColor.g}, ${selectedColor.b})`,
                            top: `${saturationPointerPos.top}%`,
                            left: `${saturationPointerPos.left}%`,
                            width: saturationFocused ? '26px' : '24px',
                            height: saturationFocused ? '26px' : '24px',
                        }"
                    />
                </div>
                <div
                    ref="hueRef"
                    relative
                    h25px
                    wfull
                    rounded-b-0.75
                    bg="[linear-gradient(to_right,rgb(255,0,0),rgb(255,255,0),rgb(0,255,0),rgb(0,255,255),rgb(0,0,255),rgb(255,0,255),rgb(255,0,0))]"
                    @mousedown="handleHueMouseDown"
                    @touchstart="handleHueMouseDown"
                >
                    <div
                        ref="huePointerRef"
                        absolute
                        left-0
                        top-0
                        z10
                        rounded-full
                        translate-x-="1/2"
                        ring="2 white"
                        :style="{
                            backgroundColor: hueColor,
                            left: `${(color.h / 360) * 100}%`,
                            width: hueFocused ? '26px' : '24px',
                            height: hueFocused ? '26px' : '24px',
                        }"
                    />
                </div>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
import Color from 'color';

import { getParentWindow } from '~/utils/getParentWindow';
import { getRelativePosition } from '~/utils/getRelativePosition';
import { getTouchId } from '~/utils/getTouchId';
import { isTouch } from '~/utils/isTouch';

const isOpen = ref(false);
const _color = defineModel<string>({ required: true, default: '#ff0000' });

const color = reactive(
    Color(_color.value).hsv().object() as {
        h: number;
        s: number;
        v: number;
    },
);

const selectedColor = computed(() => Color.hsv(color).rgb().object());
const hueColor = ref(Color.hsv(color.h, 100, 100).hex());

const saturationRef = ref<HTMLDivElement>();
const saturationPointerRef = ref<HTMLDivElement>();
const saturationPointerPos = reactive({
    top: 100 - color.v,
    left: color.s,
});
const saturationFocused = ref(false);

const hueRef = ref<HTMLDivElement>();
const huePointerRef = ref<HTMLDivElement>();
const hueFocused = ref(false);

const isKeyDown = ref(false);

watch(_color, (value) => {
    if (saturationFocused.value || hueFocused.value || isKeyDown.value) {
        return;
    }

    const colorHSV = Color(value).hsv().object();

    color.h = colorHSV.h!;
    color.s = colorHSV.s!;
    color.v = colorHSV.v!;

    hueColor.value = Color.hsv(colorHSV.h!, 100, 100).hex();

    saturationPointerPos.top = 100 - colorHSV.v!;
    saturationPointerPos.left = colorHSV.s!;
});

watch(
    color,
    (value) => {
        if (!saturationFocused.value && !hueFocused.value && !isKeyDown.value) {
            return;
        }

        hueColor.value = Color.hsv(value.h, 100, 100).hex();

        _color.value = Color.hsv(value).hex().toLowerCase();
    },
    { deep: true },
);

watch(
    saturationPointerPos,
    (value) => {
        const { top, left } = {
            top: value.top > 0 ? value.top + 6 : value.top,
            left: value.left > 0 ? value.left + 6 : value.left,
        };

        color.s = left;
        color.v = 100 - top;
    },
    { deep: true },
);

const handleSaturationPointerMove = (event: MouseEvent | TouchEvent) => {
    const { left, top } = getRelativePosition(
        saturationRef.value!,
        event,
        getTouchId(event),
    );

    saturationFocused.value = true;
    saturationPointerPos.left = clamp(left * 100, 0, 100);
    saturationPointerPos.top = clamp(top * 100, 0, 100);
};

const handleKeyDown = (event: KeyboardEvent) => {
    const newPos = {
        top: saturationPointerPos.top,
        left: saturationPointerPos.left,
    };

    const point = event.shiftKey ? 10 : 1;

    let isKey = true;

    switch (event.key) {
        case 'ArrowUp':
            newPos.top -= point;
            break;
        case 'ArrowDown':
            newPos.top += point;
            break;
        case 'ArrowLeft':
            newPos.left -= point;
            break;
        case 'ArrowRight':
            newPos.left += point;
            break;
        default:
            isKey = false;
            return;
    }

    if (isKey) {
        event.preventDefault();

        saturationFocused.value = true;
        isKeyDown.value = true;

        const affected =
            newPos.top !== saturationPointerPos.top ? 'top' : 'left';

        saturationPointerPos[affected] = clamp(newPos[affected], 0, 100);
    }
};

const handleKeyUp = (event: KeyboardEvent) => {
    let isKey = true;

    switch (event.key) {
        case 'ArrowUp':
        case 'ArrowDown':
        case 'ArrowLeft':
        case 'ArrowRight':
            break;
        default:
            isKey = false;
            return;
    }

    if (isKey) {
        event.preventDefault();

        saturationFocused.value = false;
        isKeyDown.value = false;
    }
};

const handleHuePointerMove = (event: MouseEvent | TouchEvent) => {
    const { left } = getRelativePosition(
        hueRef.value!,
        event,
        getTouchId(event),
    );

    hueFocused.value = true;
    color.h = clamp(left * 360, 0, 360);
};

const handleSaturationMouseDown = (event: MouseEvent | TouchEvent) => {
    handleSaturationPointerMove(event);

    const parentWindow = getParentWindow(saturationRef.value);
    const _isTouch = isTouch(event);

    parentWindow.addEventListener(
        _isTouch ? 'touchmove' : 'mousemove',
        handleSaturationPointerMove,
    );
    parentWindow.addEventListener(
        _isTouch ? 'touchend' : 'mouseup',
        () => {
            parentWindow.removeEventListener(
                _isTouch ? 'touchmove' : 'mousemove',
                handleSaturationPointerMove,
            );
            saturationFocused.value = false;
        },
        { once: true },
    );
};

const handleHueMouseDown = (event: MouseEvent | TouchEvent) => {
    handleHuePointerMove(event);

    const parentWindow = getParentWindow(hueRef.value);
    const _isTouch = isTouch(event);

    parentWindow.addEventListener(
        _isTouch ? 'touchmove' : 'mousemove',
        handleHuePointerMove,
    );
    parentWindow.addEventListener(
        _isTouch ? 'touchend' : 'mouseup',
        () => {
            parentWindow.removeEventListener(
                _isTouch ? 'touchmove' : 'mousemove',
                handleHuePointerMove,
            );
            hueFocused.value = false;
        },
        { once: true },
    );
};

onMounted(() => {
    window.addEventListener('keydown', handleKeyDown);
    window.addEventListener('keyup', handleKeyUp);
});

onUnmounted(() => {
    window.removeEventListener('keydown', handleKeyDown);
    window.removeEventListener('keyup', handleKeyUp);
});
</script>
