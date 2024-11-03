<template>
    <div flex="~ items-center gap2" wfull rounded bg-fs-overlay-2 p2>
        <UiButton
            v-for="(item, index) in filtered"
            :key="index"
            :variant="selected === item.label ? 'accent' : 'primary'"
            :icon="item.icon"
            :class="widthFull && 'wfull'"
            gap2
            rounded="!"
            icon-size="20"
            alignment="center"
            @click="selected = item.label"
        >
            {{ item.label }}
        </UiButton>
    </div>
</template>

<script setup lang="ts">
const { items } = defineProps<{
    items: { label: string; icon?: string; condition?: () => boolean }[];
    widthFull?: boolean;
}>();

const selected = defineModel<string>({ required: true });

const filtered = computed(() => items.filter((item) => !item.condition || item.condition()));
</script>
