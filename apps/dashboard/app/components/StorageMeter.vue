<template>
    <div class="flex flex-col gap-1.5">
        <UiProgress
            v-if="limitBytes !== null"
            :model-value="Math.min(ratio, 1) * 100"
            :aria-label="$t('storage.title')"
            :class="cn('h-1.5', ratio >= 0.9 && '*:data-[slot=progress-indicator]:bg-destructive')"
        />
        <i18n-t
            v-if="limitBytes === null"
            keypath="storage.usedUnlimited"
            tag="span"
            class="inline-flex items-center gap-1 text-xs text-muted-foreground"
        >
            <template #used>{{ formatSize(usedBytes) }}</template>
            <template #infinity>
                <IInfinity class="size-3.5" :aria-label="$t('storage.unlimited')" />
            </template>
        </i18n-t>
        <span v-else class="text-xs text-muted-foreground">
            {{ $t('storage.used', { used: formatSize(usedBytes), limit: formatSize(limitBytes) }) }}
        </span>
    </div>
</template>

<script setup lang="ts">
const { used, limit } = defineProps<{
    used: string | number;
    limit: string | number | null;
}>();

const { locale } = useI18n();

const usedBytes = computed(() => Number(used));
const limitBytes = computed(() => (limit === null ? null : Number(limit)));
const ratio = computed(() => (limitBytes.value ? usedBytes.value / limitBytes.value : 1));

const formatSize = (bytes: number) => formatStorageSize(bytes, locale.value);
</script>
