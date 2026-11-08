<template>
    <UiAlert :variant="type === 'caution' ? 'destructive' : 'default'" class="my-6">
        <component :is="ICONS[type]" :class="type !== 'caution' && 'text-chart-1'" />
        <UiAlertTitle>{{ $t(`docs.alerts.${type}`) }}</UiAlertTitle>
        <UiAlertDescription class="[&_p]:my-0 [&_p]:leading-6">
            <slot />
        </UiAlertDescription>
    </UiAlert>
</template>

<script setup lang="ts">
import type { Component } from 'vue';

import { ICircleAlert, IInfo, ILightbulb, IOctagonAlert, ITriangleAlert } from '#components';

type AlertType = 'note' | 'tip' | 'important' | 'warning' | 'caution';

const ICONS: Record<AlertType, Component> = {
    note: IInfo,
    tip: ILightbulb,
    important: ICircleAlert,
    warning: ITriangleAlert,
    caution: IOctagonAlert,
};

defineProps<{ type: AlertType }>();
</script>
