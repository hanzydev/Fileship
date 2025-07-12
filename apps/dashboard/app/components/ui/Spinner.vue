<template>
    <svg
        viewBox="0 0 16 16"
        tabindex="-1"
        aria-valuemin="0"
        aria-valuemax="100"
        :width="size"
        :height="size"
        role="progressbar"
        min-block-4
        min-inline-4
        stroke-white
        outline-none
    >
        <circle
            origin-center
            rotate--90
            fill-none
            stroke-1.5
            ease-linear
            motion-safe:transition-all
            animate="[progress-ring_linear_infinite]"
            class="[stroke-dasharray:43.97] [stroke-linecap:round]"
            :style="{
                animationDuration: duration,
            }"
            cx="50%"
            cy="50%"
            r="7"
            stroke-dasharray="3"
        />
    </svg>
</template>

<script setup lang="ts">
const { duration = '1.25s' } = defineProps<{
    size: number;
    duration?: string;
}>();
</script>

<style>
@keyframes progress-ring {
    0% {
        stroke-dasharray: 0.01px 43.97px;
        transform: rotate(0);
    }
    50% {
        stroke-dasharray: 21.99px 21.99px;
        transform: rotate(450deg);
    }
    100% {
        stroke-dasharray: 0.01px 43.97px;
        transform: rotate(1080deg);
    }
}
</style>
