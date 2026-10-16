<template>
    <div :class="cn('smart-search relative rounded-4xl', ai && 'is-active')">
        <div class="relative z-10 rounded-4xl bg-background">
            <UiInputGroup>
                <UiInputGroupAddon>
                    <UiSpinner v-if="loading" />
                    <ISparkles v-else-if="ai" class="text-primary" />
                    <ISearch v-else />
                </UiInputGroupAddon>
                <UiInputGroupInput
                    ref="input"
                    v-model="query"
                    :placeholder="
                        ai
                            ? $t('files.smartSearchPlaceholder')
                            : placeholder || $t('files.searchDefault')
                    "
                />
                <UiInputGroupAddon v-if="aiAvailable" align="inline-end">
                    <UiTooltip>
                        <UiTooltipTrigger as-child>
                            <UiInputGroupButton
                                size="icon-xs"
                                :variant="ai ? 'default' : 'ghost'"
                                :aria-label="$t('files.smartSearch')"
                                @click="ai = !ai"
                            >
                                <ISparkles />
                            </UiInputGroupButton>
                        </UiTooltipTrigger>
                        <UiTooltipContent>
                            {{ ai ? $t('files.turnOffSmartSearch') : $t('files.smartSearch') }}
                        </UiTooltipContent>
                    </UiTooltip>
                </UiInputGroupAddon>
            </UiInputGroup>
        </div>
    </div>
</template>

<script setup lang="ts">
const { placeholder } = defineProps<{
    placeholder?: string;
    loading?: boolean;
    aiAvailable?: boolean;
}>();

const query = defineModel<string>({ required: true });
const ai = defineModel<boolean>('ai', { default: false });

const input = useTemplateRef('input');

defineExpose({ focus: () => (input.value as any).$el.focus() });
</script>

<style scoped>
@property --smart-search-angle {
    syntax: '<angle>';
    initial-value: 0deg;
    inherits: false;
}

.smart-search::before,
.smart-search::after {
    content: '';
    position: absolute;
    inset: -2px;
    z-index: 0;
    border-radius: inherit;
    opacity: 0;
    transition: opacity 0.4s ease;
    background: conic-gradient(
        from var(--smart-search-angle),
        var(--chart-1),
        var(--chart-3),
        var(--chart-5),
        var(--primary),
        var(--chart-2),
        var(--chart-1)
    );
    animation: smart-search-spin 4s linear infinite;
}

.smart-search::after {
    filter: blur(12px);
}

.smart-search.is-active::before {
    opacity: 1;
}

.smart-search.is-active::after {
    opacity: 0.45;
}

@keyframes smart-search-spin {
    to {
        --smart-search-angle: 360deg;
    }
}

@media (prefers-reduced-motion: reduce) {
    .smart-search::before,
    .smart-search::after {
        animation: none;
    }
}
</style>
