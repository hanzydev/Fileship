<template>
    <div relative @mouseover="handleMouseMove" @mouseleave="handleMouseLeave">
        <div
            ref="triggerRef"
            :class="triggerClass"
            @click="handleClick"
            @contextmenu="handleContextMenu"
            @touchstart="handleIosContextMenu"
            @touchend="clearCtxMenuTimeout"
        >
            <slot />
        </div>
        <component :is="asCtxMenu ? Teleport : 'div'" to="body">
            <Transition
                enter-active-class="motion-safe:animate-in motion-safe:fade-in motion-safe:zoom-in-95 motion-safe:data-[placement=top]:slide-in-b-2 motion-safe:data-[placement=bottom]:slide-in-top-2 motion-safe:data-[placement=left]:slide-in-right-2 motion-safe:data-[placement=right]:slide-in-left-2"
                leave-active-class="motion-safe:animate-out motion-safe:fade-out motion-safe:zoom-out-95 motion-safe:data-[placement=top]:slide-out-bottom-2 motion-safe:data-[placement=bottom]:slide-out-top-2 motion-safe:data-[placement=left]:slide-out-right-2 motion-safe:data-[placement=right]:slide-out-left-2"
                :data-placement="_placement"
            >
                <div
                    v-if="isOpen"
                    v-bind="$attrs"
                    :id
                    ref="contentRef"
                    z50
                    :class="[
                        {
                            'bottom-full': _placement === 'top',
                            'top-full': _placement === 'bottom',
                            'right-full top-0': _placement === 'left',
                            'left-full top-0': _placement === 'right',
                            pb4: _placement === 'top',
                            pt4: _placement === 'bottom',
                            pl4: _placement === 'right',
                            pr4: _placement === 'left',
                        },
                        asCtxMenu ? 'fixed' : 'absolute',
                    ]"
                    :style="
                        asCtxMenu && {
                            top: `${menuPosition.y}px`,
                            left: `${menuPosition.x}px`,
                        }
                    "
                >
                    <slot name="content" />
                </div>
            </Transition>
        </component>
    </div>
</template>

<script setup lang="ts">
import { Teleport } from 'vue';

const props = withDefaults(
    defineProps<{
        placement?: 'left' | 'right' | 'top' | 'bottom';
        asCtxMenu?: boolean;
        hover?: boolean;
        triggerClass?: unknown;
    }>(),
    {
        placement: 'top',
    },
);
const {
    placement: __placement,
    asCtxMenu,
    hover,
    triggerClass,
} = toRefs(props);

defineOptions({
    inheritAttrs: false,
});

const _placement = ref(__placement.value);

const isOpen = defineModel<boolean>({ required: false, default: false });
const router = useRouter();

const triggerRef = ref<HTMLDivElement>();
const contentRef = ref<HTMLDivElement>();

const mousePosition = reactive({ x: 0, y: 0 });
const menuPosition = reactive({ x: 0, y: 0 });

const id = useId();
const isIos = useIsIos();
const overflow = useOverflow();
const activeCtxMenu = useActiveCtxMenu();

let ctxMenuTimeout: NodeJS.Timeout;
let calculatingMenuPosition = false;

const calculateMenuPosition = async () => {
    calculatingMenuPosition = true;

    isOpen.value = true;
    await nextTick();

    const { width, height } = contentRef.value!.getBoundingClientRect();

    menuPosition.x = Math.min(mousePosition.x, window.innerWidth - width - 20);
    menuPosition.y = Math.min(
        mousePosition.y,
        window.innerHeight - height - 20,
    );

    if (menuPosition.y < 0) menuPosition.y = 0;

    calculatingMenuPosition = false;
    isOpen.value = false;
};

const handleContextMenu = async (event: MouseEvent | TouchEvent) => {
    if (!asCtxMenu.value || hover.value) return;

    event.preventDefault();
    event.stopPropagation();

    mousePosition.x =
        'clientX' in event
            ? event.clientX
            : (event.touches[0]?.clientX as number);
    mousePosition.y =
        'clientY' in event
            ? event.clientY
            : (event.touches[0]?.clientY as number);

    if (menuPosition.x !== 0 && menuPosition.y !== 0) {
        isOpen.value = false;
        await nextTick();
    }

    await calculateMenuPosition();
    isOpen.value = true;
};

const handleIosContextMenu = (event: TouchEvent) => {
    if (!asCtxMenu.value || hover.value || !isIos.value) return;

    clearTimeout(ctxMenuTimeout);
    ctxMenuTimeout = setTimeout(() => {
        handleContextMenu(event);
    }, 300);
};

const clearCtxMenuTimeout = () => clearTimeout(ctxMenuTimeout);

const handleClick = () => {
    if (asCtxMenu.value || hover.value) return;

    isOpen.value = !isOpen.value;
};

const handleMouseMove = () => {
    if (hover.value && !asCtxMenu.value && !isOpen.value) isOpen.value = true;
};

const handleMouseLeave = () => {
    if (hover.value && !asCtxMenu.value && isOpen.value) isOpen.value = false;
};

const preventOverflow = async () => {
    if (_placement.value !== __placement.value) return;

    const rect = contentRef.value!.getBoundingClientRect();

    const space = {
        left: rect.left,
        right: window.innerWidth - rect.right,
        top: rect.top,
        bottom: window.innerHeight - rect.bottom,
    };

    let newPlacement = _placement.value;

    if (space.left < 0) newPlacement = 'right';
    if (space.right < 0) newPlacement = 'left';
    if (space.top < 0) newPlacement = 'bottom';
    if (space.bottom < 0) newPlacement = 'top';

    if (window.innerWidth < 768 && (space.right < 0 || space.left < 0)) {
        if (space.top < 0 || space.bottom < 0) newPlacement = 'top';
        else newPlacement = 'bottom';
    }

    _placement.value = newPlacement;
};

onClickOutside(
    contentRef,
    (event) => {
        const activeCtx = document.getElementById(activeCtxMenu.value!);

        if (isOpen.value && !activeCtx?.contains(event.target as Node)) {
            isOpen.value = false;
        }
    },
    {
        ignore: asCtxMenu.value ? [] : [triggerRef],
    },
);

onKeyStroke(
    'Escape',
    () => {
        if (isOpen.value) isOpen.value = false;
    },
    { eventName: 'keydown' },
);

onUnmounted(() => asCtxMenu.value && (overflow.value = true));

watch(isOpen, async (value) => {
    if (calculatingMenuPosition) return;

    if (asCtxMenu.value) activeCtxMenu.value = value ? id : '';
    overflow.value = !activeCtxMenu.value;

    if (value && !asCtxMenu.value) nextTick(preventOverflow);
});

watch(activeCtxMenu, (value) => {
    if (asCtxMenu.value && value !== id && isOpen.value) isOpen.value = false;
});

watch(__placement, (value) => (_placement.value = value));

router.beforeEach((_, __, next) => {
    if (isOpen.value) isOpen.value = false;
    next();
});
</script>
