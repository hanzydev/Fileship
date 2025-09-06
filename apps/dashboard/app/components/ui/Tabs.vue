<template>
    <div
        flex="~ col sm:(wrap row) gap1"
        wfull
        rounded-lg
        p1
        border="~ fs-overlay-4"
        :class="{
            'bg-fs-overlay-2': variant === 'primary',
            'bg-fs-overlay-3': variant === 'secondary',
        }"
    >
        <UiButton
            v-for="(item, index) in filtered"
            :key="index"
            :variant="selected === item.label ? 'accent' : 'ghost'"
            :icon="item.icon"
            gap2
            rounded-md="!"
            icon-size="20"
            :alignment="width > 640 ? 'center' : 'left'"
            :disabled
            :class="[
                buttonClass,
                widthFull && 'wfull! flex-1',
                selected !== item.label &&
                    (variant === 'primary' ? 'hover:bg-fs-overlay-3' : 'hover:bg-fs-overlay-4'),
            ]"
            @click="selected = item.label"
        >
            {{ item.label }}
        </UiButton>
    </div>
</template>

<script setup lang="ts">
const { variant = 'primary', items } = defineProps<{
    items: { label: string; icon?: string; condition?: () => boolean }[];
    widthFull?: boolean;
    variant?: 'primary' | 'secondary';
    buttonClass?: unknown;
    disabled?: boolean;
}>();

const selected = defineModel<string>({ required: true });

const filtered = computed(() => items.filter((item) => !item.condition || item.condition()));

const { width } = useWindowSize();
</script>
