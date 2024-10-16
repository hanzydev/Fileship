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
            bg-fs-overlay-3
            px3.5
            py2.5
            outline-none
            motion-safe:transition-shadow
            placeholder-slate-300
            ring="1 fs-overlay-4"
            :class="[
                disabled || $props.readonly
                    ? 'cursor-not-allowed'
                    : 'focus:ring-fs-accent',
            ]"
            :rows="lines > 2 ? lines : 2"
            :required
            :disabled
            :readonly
        />
    </div>
</template>

<script setup lang="ts">
defineProps<{
    error?: string;
    label?: string;
    required?: boolean;
    disabled?: boolean;
    readonly?: boolean;
}>();

defineOptions({
    inheritAttrs: false,
});

const value = defineModel<string>({ required: true });

const id = useId();

const lines = computed(() => value.value.split('\n').length);
</script>
