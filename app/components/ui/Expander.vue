<template>
    <div z20 wfull rounded-xl bg-fs-overlay-2 ring="1 fs-overlay-4">
        <div
            flex="~ items-center justify-between"
            cursor-pointer
            select-none
            p4
            @click="isOpen = !isOpen"
        >
            <slot />
            <UiButton
                h8
                w8
                rounded-md="!"
                pl="!"
                variant="primary"
                ring="1 fs-overlay-4"
                bg-fs-overlay-3
                hover="!ring-none !bg-fs-overlay-4"
                alignment="center"
                icon="heroicons-solid:chevron-down"
                icon-size="20"
                :icon-class="['motion-safe:transition-transform', isOpen && 'rotate-x-180']"
            />
        </div>
        <Transition
            enter-from-class="h0"
            leave-to-class="h0"
            enter-active-class="motion-safe:(animate-in fade-in transition-height) overflow-hidden"
            leave-active-class="motion-safe:(animate-out fade-out transition-height) overflow-hidden"
            enter-to-class="h[--height]"
            leave-from-class="h[--height]"
            :style="{ '--height': `${height}px` }"
        >
            <div v-if="isOpen" ref="content" wfull rounded-b-xl bg-fs-overlay-2>
                <div px4 pb4 :class="contentClass">
                    <slot name="content" />
                </div>
            </div>
        </Transition>
    </div>
</template>

<script setup lang="ts">
const isOpen = defineModel<boolean>({ required: false, default: false });

defineProps<{
    contentClass?: unknown;
}>();

const contentRef = useTemplateRef<HTMLDivElement>('content');
const height = ref(0);

watch(isOpen, async (value) => {
    if (value) {
        await nextTick();
        height.value = contentRef.value!.scrollHeight;
    }
});
</script>
