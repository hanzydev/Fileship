<template>
    <div :class="wrapperClass" space-y-1>
        <UiLabel v-if="label" :for="id" :required :error>
            {{ label }}
        </UiLabel>
        <textarea
            v-bind="$attrs"
            :id="label && id"
            v-model="value"
            rounded-lg
            bg-fs-overlay-3
            px3.5
            py2.5
            outline-none
            motion-safe:transition-shadow
            placeholder-fs-muted-2
            ring="1 fs-overlay-4"
            :class="[disabled || $props.readonly ? 'cursor-not-allowed' : 'focus:ring-fs-accent']"
            :rows
            :required
            :disabled
            :readonly
        />
    </div>
</template>

<script setup lang="ts">
import type { ClassValue } from 'vue';

const { readonly = false } = defineProps<{
    error?: string;
    label?: string;
    required?: boolean;
    disabled?: boolean;
    readonly?: boolean;
    wrapperClass?: ClassValue;
}>();

defineOptions({
    inheritAttrs: false,
});

const value = defineModel<string>({ required: true });

const id = useId();

const rows = computed(() => {
    const lines = value.value.split('\n').length;
    return clamp(lines, 10, 20);
});
</script>
