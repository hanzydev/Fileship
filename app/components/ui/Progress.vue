<template>
    <div flex="~ gap2 items-center">
        <span text-sm="!" :class="textClass">{{ value }}%</span>
        <svg h6 w6 viewBox="0 0 100 100" xmlns="http://www.w3.org/2000/svg">
            <circle
                cx="50"
                cy="50"
                r="45"
                fill="none"
                stroke-fs1
                stroke-width="10"
            />
            <circle
                cx="50"
                cy="50"
                r="45"
                fill="none"
                stroke-fs-accent
                stroke-width="10"
                :stroke-dasharray="282.7433388230814"
                :stroke-dashoffset="282.7433388230814 * (1 - value / 100)"
            />
        </svg>
    </div>
</template>

<script setup lang="ts">
defineProps<{ value: number; textClass?: unknown }>();
</script>
