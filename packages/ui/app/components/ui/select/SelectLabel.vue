<script setup lang="ts">
import type { SelectLabelProps } from 'reka-ui';
import { SelectLabel } from 'reka-ui';
import type { HTMLAttributes } from 'vue';

const props = defineProps<SelectLabelProps & { class?: HTMLAttributes['class'] }>();
</script>

<template>
    <SelectLabel
        data-slot="select-label"
        :class="cn('text-muted-foreground px-3 py-2.5 text-xs', props.class)"
    >
        <slot />
    </SelectLabel>
</template>
