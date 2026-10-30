<template>
    <div class="flex shrink-0 items-center">
        <UiButton
            v-if="index > 0"
            variant="ghost"
            size="icon-sm"
            :aria-label="$t('sharing.discord.moveUp')"
            @click="emit('move', -1)"
        >
            <IChevronUp />
        </UiButton>
        <UiButton
            v-if="index < length - 1"
            variant="ghost"
            size="icon-sm"
            :aria-label="$t('sharing.discord.moveDown')"
            @click="emit('move', 1)"
        >
            <IChevronDown />
        </UiButton>
        <UiButton
            v-if="canCopy"
            variant="ghost"
            size="icon-sm"
            :aria-label="$t('sharing.discord.copy')"
            @click="emit('copy')"
        >
            <ICopy />
        </UiButton>
        <UiButton
            v-if="canRemove"
            variant="ghost"
            size="icon-sm"
            :aria-label="$t('sharing.discord.remove')"
            @click="emit('remove')"
        >
            <ITrash2 />
        </UiButton>
    </div>
</template>

<script setup lang="ts">
const {
    index,
    length,
    canCopy,
    canRemove = true,
} = defineProps<{
    index: number;
    length: number;
    canCopy: boolean;
    canRemove?: boolean;
}>();

const emit = defineEmits<{ move: [offset: number]; copy: []; remove: [] }>();
</script>
