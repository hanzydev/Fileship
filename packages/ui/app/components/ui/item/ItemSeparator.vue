<script setup lang="ts">
import type { SeparatorProps } from 'reka-ui';
import type { HTMLAttributes } from 'vue';

import { Separator } from '#layers/ui/app/components/ui/separator';
import { cn } from '#layers/ui/app/utils';

const props = defineProps<SeparatorProps & { class?: HTMLAttributes['class'] }>();
</script>

<template>
    <Separator
        data-slot="item-separator"
        orientation="horizontal"
        :class="cn('my-2', props.class)"
    />
</template>
