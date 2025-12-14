<template>
    <Teleport to="body">
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in)"
            leave-active-class="motion-safe:(animate-out fade-out)"
            @after-leave="$emit('closed')"
        >
            <div
                v-if="isOpen"
                text-left="!"
                bg="black/60"
                fixed
                inset-0
                z40
                backdrop-blur-sm
                :class="backgroundClass"
                v-bind="backgroundProps"
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
                z50
                max-w-640px
                overflow-auto
                rounded-2xl
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
const { closeOnOuterClick = true, closable = true } = defineProps<{
    closeOnOuterClick?: boolean;
    closable?: boolean;
    backgroundClass?: unknown;
    backgroundProps?: { [key: string]: unknown };
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

const contentRef = useTemplateRef<HTMLDivElement>('content');

onClickOutside(
    contentRef,
    async () => {
        if (closeOnOuterClick && isOpen.value && closable) isOpen.value = false;
        emit('outerClick');
    },
    {
        ignore: ['[data-ignore-modal-outer-click]'],
    },
);

onKeyStroke(
    'Escape',
    () => {
        if (isOpen.value && closable) isOpen.value = false;
    },
    { eventName: 'keydown' },
);

router.beforeEach((_, __, next) => {
    if (isOpen.value) isOpen.value = false;
    next();
});
</script>
