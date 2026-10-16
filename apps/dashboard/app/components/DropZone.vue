<template>
    <UiEmpty ref="root" :class="cn('border border-dashed', isOver && !disabled && 'bg-muted/50')">
        <UiEmptyHeader>
            <UiEmptyMedia variant="icon">
                <IUpload />
            </UiEmptyMedia>
            <UiEmptyTitle>{{ $t('layout.dropZone.title') }}</UiEmptyTitle>
            <UiEmptyDescription v-if="description">{{ description }}</UiEmptyDescription>
        </UiEmptyHeader>
        <UiEmptyContent>
            <UiButton variant="outline" size="sm" :disabled @click="input?.click()">
                {{ $t('layout.dropZone.browse') }}
            </UiButton>
        </UiEmptyContent>
        <input
            ref="input"
            type="file"
            multiple
            class="hidden"
            :disabled="disabled"
            @change="handleInput"
        />
    </UiEmpty>
</template>

<script setup lang="ts">
const { disabled } = defineProps<{
    description?: string;
    disabled?: boolean;
}>();

const emit = defineEmits<{
    files: [files: File[]];
}>();

const root = useTemplateRef<any>('root');
const input = useTemplateRef('input');

const { isOverDropZone: isOver } = useDropZone(
    computed(() => root.value?.$el),
    {
        onDrop: (files) => !disabled && files?.length && emit('files', files),
        checkValidity: (items) => [...items].some((item) => item.kind === 'file'),
        multiple: true,
    },
);

const handleInput = (event: Event) => {
    const target = event.target as HTMLInputElement;
    if (target.files?.length) emit('files', [...target.files]);
    target.value = '';
};
</script>
