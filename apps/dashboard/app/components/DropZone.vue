<template>
    <label ref="root" :class="cn('block', disabled && 'pointer-events-none opacity-50')">
        <UiEmpty
            :class="
                cn(
                    'border border-dashed transition-colors',
                    isOver ? 'border-primary bg-primary/5' : 'hover:bg-muted/50',
                )
            "
        >
            <UiEmptyHeader>
                <UiEmptyMedia variant="icon">
                    <IUpload />
                </UiEmptyMedia>
                <UiEmptyTitle>Drop files here or click to browse</UiEmptyTitle>
                <UiEmptyDescription v-if="description">{{ description }}</UiEmptyDescription>
            </UiEmptyHeader>
        </UiEmpty>
        <input type="file" multiple class="sr-only" :disabled="disabled" @change="handleInput" />
    </label>
</template>

<script setup lang="ts">
const { disabled } = defineProps<{
    description?: string;
    disabled?: boolean;
}>();

const emit = defineEmits<{
    files: [files: File[]];
}>();

const root = useTemplateRef('root');

const { isOverDropZone: isOver } = useDropZone(root, {
    onDrop: (files) => files?.length && emit('files', files),
    checkValidity: (items) => [...items].some((item) => item.kind === 'file'),
    multiple: true,
});

const handleInput = (event: Event) => {
    const input = event.target as HTMLInputElement;
    if (input.files?.length) emit('files', [...input.files]);
    input.value = '';
};
</script>
