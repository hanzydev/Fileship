<template>
    <div flex="~ shrink-0" relative>
        <img
            v-if="src && !error"
            v-bind="$attrs"
            :src
            :alt
            :class="[size === 'xs' && 'h8 w8', size === 'sm' && 'h9 w9']"
            select-none
            rounded-full
            bg-fs-overlay-3
            ring="1 fs-overlay-4"
            draggable="false"
            @error="error = true"
        />
        <div
            v-else
            v-bind="$attrs"
            flex="~ items-center justify-center"
            select-none
            rounded-full
            bg-fs-overlay-3
            ring="1 fs-overlay-4"
            font-medium
            :class="[size === 'xs' && 'h8 w8', size === 'sm' && 'h9 w9']"
        >
            {{
                alt
                    ?.split(' ')
                    .map((i) => i.charAt(0))
                    .join('')
                    .toUpperCase() ?? ''
            }}
        </div>
    </div>
</template>

<script setup lang="ts">
defineProps<{
    size: 'xs' | 'sm';
    src?: string;
    alt: string;
}>();

const error = ref(false);

defineOptions({
    inheritAttrs: false,
});
</script>
