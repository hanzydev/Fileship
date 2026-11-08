<template>
    <div class="my-6 overflow-hidden rounded-3xl border bg-card">
        <div class="flex items-center justify-between gap-2 border-b py-1.5 pr-1.5 pl-4">
            <span class="truncate font-mono text-xs text-muted-foreground">{{ label }}</span>
            <UiButton
                variant="ghost"
                size="icon-sm"
                :aria-label="copied ? $t('docs.code.copied') : $t('docs.code.copy')"
                @click="handleCopy"
            >
                <ICheck v-if="copied" />
                <ICopy v-else />
            </UiButton>
        </div>
        <pre
            :class="
                cn(
                    'overflow-x-auto bg-transparent! p-4 font-mono text-[13px] leading-6',
                    $props.class,
                )
            "
            :style="style"
        ><slot /></pre>
    </div>
</template>

<script setup lang="ts">
const props = defineProps<{
    code?: string;
    language?: string | null;
    filename?: string | null;
    class?: string | null;
    style?: string | Record<string, string> | null;
}>();

const label = computed(() => props.filename || props.language || 'text');

const { copy, copied } = useClipboard({ legacy: true });

const handleCopy = () => copy(props.code ?? '');
</script>
