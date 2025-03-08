<template>
    <div grid="~ sm:cols-3 gap2" wfull rounded bg-fs-overlay-2 p2>
        <UiButton
            v-for="(item, index) in filtered"
            :key="index"
            :variant="selected === item.label ? 'accent' : 'onOverlay'"
            :icon="item.icon"
            :class="widthFull && 'wfull'"
            gap2
            rounded="!"
            icon-size="20"
            :alignment="width > 768 ? 'center' : 'left'"
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

const { width } = useWindowSize();
</script>
