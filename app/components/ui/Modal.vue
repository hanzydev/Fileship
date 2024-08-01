<template>
    <Teleport to="body">
        <Transition
            enter-active-class="motion-safe:animate-in motion-safe:fade-in"
            leave-active-class="motion-safe:animate-out motion-safe:fade-out"
            @after-leave="$emit('closed')"
        >
            <div
                v-if="isOpen"
                bg-black="/20"
                text-left="!"
                fixed
                inset-0
                z40
                backdrop-blur-sm
            ></div>
        </Transition>
        <Transition
            enter-active-class="motion-safe:animate-in overflow-hidden motion-safe:fade-in motion-safe:zoom-in-95 motion-safe:slide-in-left-1/2 motion-safe:slide-in-top-48%"
            leave-active-class="motion-safe:animate-out overflow-hidden motion-safe:fade-out motion-safe:zoom-out-95 motion-safe:slide-out-left-1/2 motion-safe:slide-out-top-48%"
        >
            <div
                v-if="isOpen"
                v-bind="$attrs"
                ref="contentRef"
                ring="1 fs-accent"
                fixed
                z40
                max-w-640px
                overflow-auto
                rounded-lg
                bg-fs-overlay-1
                left="1/2"
                top="1/2"
                max-h="90%"
                w="90%"
                translate-x="-1/2"
                translate-y="-1/2"
            >
                <slot />
            </div>
        </Transition>
    </Teleport>
</template>

<script setup lang="ts">
const { closeOnOuterClick } = withDefaults(
    defineProps<{
        closeOnOuterClick?: boolean;
    }>(),
    {
        closeOnOuterClick: true,
    },
);

defineOptions({
    inheritAttrs: false,
});

const emit = defineEmits<{
    outerClick: [];
    closed: [];
}>();

const isOpen = defineModel<boolean>({ required: true });

const router = useRouter();
const overflow = useOverflow();

const contentRef = ref<HTMLDivElement>();

onClickOutside(contentRef, async () => {
    if (closeOnOuterClick && isOpen.value) isOpen.value = false;
    emit('outerClick');
});

onKeyStroke(
    'Escape',
    () => {
        if (isOpen.value) isOpen.value = false;
    },
    { eventName: 'keydown' },
);

onUnmounted(() => (overflow.value = true));

watch(isOpen, (value) => (overflow.value = !value));

router.beforeEach((_, __, next) => {
    if (isOpen.value) isOpen.value = false;
    next();
});
</script>
