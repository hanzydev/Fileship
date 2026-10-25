<template>
    <img
        v-if="src"
        :src="src"
        :alt="''"
        loading="lazy"
        decoding="async"
        draggable="false"
        :class="
            cn(
                'size-full object-cover',
                isPdf(file.mimeType) && 'object-top',
                isSensitive && 'blur-sm',
            )
        "
    />
    <component :is="getFileIcon(file)" v-else class="size-[42%] text-muted-foreground" />
    <IPlay v-if="isVideoFile(file)" class="absolute size-1/3 fill-white text-white drop-shadow" />
</template>

<script setup lang="ts">
import type { FileData } from '~/types/files';

const { file } = defineProps<{
    file: FileData;
}>();

const currentUser = useAuthUser();
const publicSettings = usePublicSettings();

const src = computed(
    () => file.thumbnailUrl || (file.mimeType.startsWith('image/') ? file.directUrl : null),
);

const isSensitive = computed(
    () =>
        file.mimeType.startsWith('image/') &&
        file.piiDetected &&
        currentUser.value!.aiSettings.suppressPii &&
        publicSettings.value.aiEnabled,
);
</script>
