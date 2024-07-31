<template>
    <section
        border="l t fs-background"
        wfull
        bg-fs-overlay-1
        p6
        lg:ml64
        lg:w="[calc(100%-256px)]"
        lg:rounded-tl-lg
        :style="{
            height: `calc(100vh - 70px${adminSessionId ? ' - 3rem' : ''})`,
        }"
        :class="overflow ? 'overflow-x-hidden' : 'overflow-hidden'"
    >
        <slot />
    </section>
</template>

<script setup lang="ts">
const overflow = useOverflow();
const adminSessionId = useCookie('adminSessionId');
</script>
