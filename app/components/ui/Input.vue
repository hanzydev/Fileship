<template>
    <div space-y-1>
        <UiLabel v-if="label" :for="id" :required :error>
            {{ label }}
        </UiLabel>
        <div relative>
            <input
                v-bind="$attrs"
                :id="label && id"
                ref="inputRef"
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
                        'bg-fs2': variant === 'primary',
                        'bg-fs1': variant === 'secondary',
                        pr11: $attrs.type === 'password',
                    },
                ]"
                :required
                :disabled
                :readonly
                :type="
                    passwordVisible || !$attrs.type
                        ? 'text'
                        : ($attrs.type as never)
                "
            />

            <UiButton
                v-if="$attrs.type === 'password'"
                absolute
                right-2
                top-2
                rounded
                motion-safe:transition-all
                p1="!"
                :class="!readonly && !(value as any)?.length && 'invisible op0'"
                :icon="
                    passwordVisible
                        ? 'heroicons-solid:eye'
                        : 'heroicons-solid:eye-off'
                "
                icon-size="20"
                variant="secondary"
                aria-label="Toggle password visibility"
                :disabled="disabled || readonly"
                @click="passwordVisible = !passwordVisible"
            />

            <div
                v-if="$attrs.type === 'number'"
                absolute
                right-0
                top-0
                hfull
                rounded-r-md
                bg-fs1
                flex="~ col items-center justify-center"
            >
                <UiButton
                    rounded="none tr-md"
                    bg-transparent
                    p1="!"
                    icon="heroicons-solid:chevron-up"
                    aria-label="Increment value"
                    :disabled="
                        disabled || readonly || (value as number) >= max!
                    "
                    @click="(value as number)++"
                />
                <UiButton
                    rounded="none br-md"
                    bg-transparent
                    p1="!"
                    icon="heroicons-solid:chevron-down"
                    aria-label="Decrement value"
                    :disabled="
                        disabled || readonly || (value as number) <= min!
                    "
                    @click="(value as number)--"
                />
            </div>
        </div>

        <p v-if="caption" text-slate300 text-sm="!" font-medium="!">
            {{ caption }}
        </p>
    </div>
</template>

<script setup lang="ts">
const {
    min,
    max,
    variant = 'primary',
} = defineProps<{
    variant?: 'primary' | 'secondary';
    error?: string;
    label?: string;
    required?: boolean;
    disabled?: boolean;
    readonly?: boolean;
    min?: number;
    max?: number;
    caption?: string;
}>();

defineOptions({
    inheritAttrs: false,
});

const value = defineModel<number | string>({ required: true });

const id = useId();
const inputRef = ref<HTMLInputElement>();
const passwordVisible = ref(false);

watch(value, (newValue) => {
    if (typeof newValue === 'number') {
        value.value = Math.max(min ?? 0, Math.min(max ?? Infinity, newValue));
    }
});
</script>
