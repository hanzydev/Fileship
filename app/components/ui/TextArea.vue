<template>
    <div space-y-1>
        <UiLabel v-if="label" :for="id" :required :error>
            {{ label }}
        </UiLabel>
        <textarea
            v-bind="$attrs"
            :id="label && id"
            v-model="value"
            rounded-md
            px3.5
            py2.5
            outline-none
            motion-safe:transition-shadow
            placeholder-slate-300
            :class="[
                disabled || $props.readonly
                    ? 'cursor-not-allowed'
                    : 'focus:ring-1 focus:ring-fs-accent',
                {
                    'bg-fs-overlay-3': variant === 'primary',
                    'bg-fs-overlay-4': variant === 'secondary',
                },
            ]"
            :rows="value.split('\n').length"
            :required
            :disabled
            :readonly
        />
    </div>
</template>

<script setup lang="ts">
withDefaults(
    defineProps<{
        variant?: 'primary' | 'secondary';
        error?: string;
        label?: string;
        required?: boolean;
        disabled?: boolean;
        readonly?: boolean;
    }>(),
    { variant: 'primary' },
);

defineOptions({
    inheritAttrs: false,
});

const value = defineModel<string>({ required: true });

const id = useId();
</script>
