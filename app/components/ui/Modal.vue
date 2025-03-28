<template>
    <Teleport to="body">
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in)"
            leave-active-class="motion-safe:(animate-out fade-out)"
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
            enter-active-class="motion-safe:(animate-in overflow-hidden fade-in zoom-in-95 slide-in-left-1/2 slide-in-top-48%)"
            leave-active-class="motion-safe:(animate-out overflow-hidden fade-out zoom-out-95 slide-out-left-1/2 slide-out-top-48%)"
        >
            <div
                v-if="isOpen"
                v-bind="$attrs"
                ref="content"
                ring="1 fs-overlay-4"
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
const { closeOnOuterClick = true } = defineProps<{
    closeOnOuterClick?: boolean;
}>();

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

const contentRef = useTemplateRef<HTMLDivElement>('content');

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
