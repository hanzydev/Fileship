<template>
    <div w="[calc(100%-256px-48px-24px)]" mt6 space-y-6>
        <section
            flex="~ items-center justify-between gap-4"
            border="~ fs-overlay-3"
            h5rem
            rounded-2xl
            bg-fs-overlay-1
            px-6
        >
            <slot name="header" />
        </section>
        <section
            border="~ fs-overlay-3"
            wfull
            rounded-2xl
            bg-fs-overlay-1
            p6
            space-y-6
            motion-safe="transition-max-height"
            :class="overflow ? 'overflow-x-hidden' : 'overflow-hidden'"
            :style="{
                maxHeight: `calc(100vh - 3rem - 5rem - 24px${adminSessionId ? ' - 72px' : ''})`,
            }"
        >
            <slot />
        </section>
    </div>
</template>

<script setup lang="ts">
const overflow = useOverflow();
const adminSessionId = useCookie('adminSessionId');
</script>
