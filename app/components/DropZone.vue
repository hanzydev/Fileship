<template>
    <div flex="~ col gap4" wfull>
        <div
            border="~ 2 dashed rounded-md fs-accent"
            relative
            h48
            wfull
            overflow-hidden
            active:scale-95
            motion-safe:transition-transform
            :class="inputDisabled && 'op50'"
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
                :accept="accept.join(',')"
                :disabled="inputDisabled"
                @change.stop.prevent="handleUpload"
            />
            <div
                flex="~ justify-center"
                absolute
                top-0
                hfull
                wfull
                bg-fs-overlay-2
            >
                <div flex="~ items-center justify-center col gap4">
                    <Icon name="heroicons-solid:cloud-upload" size="40" />
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
} = defineProps<{
    accept?: string[];
    max?: number;
    multiple?: boolean;
    placeholder?: string;
    disabled?: boolean;
}>();

const files = defineModel<File[]>({ required: true });

const inputDisabled = computed(() => _disabled || files.value.length >= max);

const handleUpload = async (event: Event) => {
    const newFiles = Array.from(
        (event.target as HTMLInputElement).files!,
    ).filter((f) => !files.value.map((f) => f.name).includes(f.name));

    files.value = [...files.value, ...newFiles].slice(0, max);
};
</script>
