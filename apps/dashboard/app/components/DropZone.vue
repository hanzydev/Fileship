<template>
    <div flex="~ col gap4" wfull>
        <div
            border="~ 2 dashed fs-accent"
            relative
            h48
            wfull
            overflow-hidden
            rounded-lg
            active:scale-95
            motion-safe:transition-all
            :class="[
                inputDisabled && 'op50',
                {
                    'bg-fs-overlay-3 hover:bg-fs-overlay-4': variant === 'primary',
                    'bg-fs-overlay-2 hover:bg-fs-overlay-3': variant === 'secondary',
                },
            ]"
        >
            <input
                :key="files.length"
                absolute
                inset-0
                z10
                hfull
                wfull
                op0
                :class="inputDisabled ? 'cursor-not-allowed' : 'cursor-pointer'"
                type="file"
                :multiple
                :accept="accept?.join(',')"
                :disabled="inputDisabled"
                @change.stop.prevent="handleUpload"
            />
            <div flex="~ justify-center" absolute top-0 hfull wfull>
                <div flex="~ items-center justify-center col gap4">
                    <Icon name="solar:cloud-upload-bold" size="40" />
                    <h5>Drag and drop {{ placeholder }} here</h5>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
const {
    accept = [],
    max = Infinity,
    multiple = true,
    placeholder = 'file(s)',
    disabled: _disabled = false,
    variant = 'primary',
} = defineProps<{
    accept?: string[];
    max?: number;
    multiple?: boolean;
    placeholder?: string;
    disabled?: boolean;
    variant?: 'primary' | 'secondary';
}>();

const files = defineModel<File[]>({ required: true });

const inputDisabled = computed(() => _disabled || files.value.length >= max);

const handleUpload = async (event: Event) => {
    const newFiles = Array.from((event.target as HTMLInputElement).files!).filter(
        (f) => !files.value.map((f) => f.name).includes(f.name),
    );

    files.value = [...files.value, ...newFiles].slice(0, max);
};
</script>
