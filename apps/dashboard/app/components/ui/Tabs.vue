<template>
    <div grid="~ sm:cols-3 gap2" wfull rounded-lg bg-fs-overlay-3 p1.5 ring="1 fs-overlay-4">
        <UiButton
            v-for="(item, index) in filtered"
            :key="index"
            :variant="selected === item.label ? 'accent' : 'ghost'"
            :icon="item.icon"
            :class="[widthFull && 'wfull', selected !== item.label && 'hover:bg-fs-overlay-4']"
            gap2
            rounded-md="!"
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
