<template>
    <div class="skeleton" data-allow-mismatch>
        <slot />
    </div>
</template>

<script setup lang="ts">
defineProps<{
    colorSteps: [string, string];
}>();
</script>

<style scoped>
@keyframes skeletonLoad {
    0% {
        background-position: 125% 0%;
    }
    30%,
    100% {
        background-position: -25% 0%;
    }
}

.skeleton {
    background-image: linear-gradient(
        90deg,
        v-bind('colorSteps?.[0]') 25%,
        v-bind('colorSteps?.[1]') 50%,
        v-bind('colorSteps?.[0]') 75%
    );
    animation: skeletonLoad 1.5s infinite cubic-bezier(0, 0, 0.58, 1);
    @apply bg-[size:300%_100%] bg-[color:unset]!;
}
</style>
