<template>
    <UiDropdown v-model="isOpen" placement="top" pb0.5="!">
        <slot />
        <template #content>
            <div
                relative
                top-6
                z10
                w200px
                rounded
                bg-fs-overlay-2
                p2
                ring-1
                :style="{
                    '--un-ring-color': _color,
                }"
            >
                <div
                    ref="saturation"
                    relative
                    h164px
                    wfull
                    rounded-t-0.75
                    bg="[linear-gradient(0deg,#000,transparent),linear-gradient(90deg,#fff,hsla(0,0%,100%,0))]"
                    :style="{
                        backgroundColor: hueColor,
                    }"
                    @mousedown.prevent="
                        (event) => {
                            saturationFocused = true;
                            handleSaturationMove(event);
                        }
                    "
                    @mousemove.prevent="handleSaturationMove"
                    @mouseup.prevent="saturationFocused = false"
                    @touchstart.prevent="
                        (event) => {
                            saturationFocused = true;
                            handleSaturationMove(event);
                        }
                    "
                    @touchmove.prevent="handleSaturationMove"
                    @touchend.prevent="saturationFocused = false"
                >
                    <div
                        absolute
                        right-0
                        top-0
                        z10
                        rounded-full
                        translate-x-="1/2"
                        translate-y-="1/2"
                        ring="1 white"
                        :style="{
                            backgroundColor: _color,
                            top: `${saturationPointerPos.top}%`,
                            left: `${saturationPointerPos.left}%`,
                            width: saturationFocused ? '26px' : '24px',
                            height: saturationFocused ? '26px' : '24px',
                        }"
                    />
                </div>
                <div
                    ref="hue"
                    relative
                    h25px
                    wfull
                    rounded-b-0.75
                    bg="[linear-gradient(to_right,rgb(255,0,0),rgb(255,255,0),rgb(0,255,0),rgb(0,255,255),rgb(0,0,255),rgb(255,0,255),rgb(255,0,0))]"
                    @mousedown.prevent="
                        (event) => {
                            hueFocused = true;
                            handleHueMove(event);
                        }
                    "
                    @mousemove.prevent="handleHueMove"
                    @mouseup.prevent="hueFocused = false"
                    @touchstart.prevent="
                        (event) => {
                            hueFocused = true;
                            handleHueMove(event);
                        }
                    "
                    @touchmove.prevent="handleHueMove"
                    @touchend.prevent="hueFocused = false"
                >
                    <div
                        absolute
                        left-0
                        top-0
                        z10
                        rounded-full
                        translate-x-="1/2"
                        ring="1 white"
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

const isOpen = ref(false);
const _color = defineModel<string>({ required: true, default: '#ff0000' });

const color = reactive(
    Color(_color.value).hsv().object() as {
        h: number;
        s: number;
        v: number;
    },
);

const saturationRef = useTemplateRef('saturation');
const hueRef = useTemplateRef('hue');

const saturationPointerPos = reactive({
    top: 100 - color.v,
    left: color.s,
});

const hueColor = ref(Color.hsv(color.h, 100, 100).hex());

const hueFocused = ref(false);
const saturationFocused = ref(false);

const handleSaturationMove = (event: MouseEvent | TouchEvent) => {
    if (!saturationFocused.value) return;

    const { left, top } = getRelativePosition(
        saturationRef.value as never,
        event,
        getTouchId(event),
    );

    saturationPointerPos.left = clamp(left * 100, 0, 100);
    saturationPointerPos.top = clamp(top * 100, 0, 100);
};

const handleHueMove = (event: MouseEvent | TouchEvent) => {
    if (!hueFocused.value) return;

    const { left } = getRelativePosition(hueRef.value as never, event, getTouchId(event));

    color.h = clamp(left * 360, 0, 360);
};

watch(_color, (value) => {
    if (saturationFocused.value || hueFocused.value) return;

    const colorHSV = Color(value).hsv().object();

    colorHSV.h = colorHSV.h!;
    colorHSV.s = colorHSV.s!;
    colorHSV.v = colorHSV.v!;

    hueColor.value = Color.hsv(colorHSV.h!, 100, 100).hex();

    saturationPointerPos.top = 100 - colorHSV.v!;
    saturationPointerPos.left = colorHSV.s!;
});

watch(
    color,
    (value) => {
        if (!saturationFocused.value && !hueFocused.value) return;

        _color.value = Color.hsv(value).hex().toLowerCase();
        hueColor.value = Color.hsv(value.h, 100, 100).hex();
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
</script>
