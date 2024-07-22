<template>
    <UiModal
        v-if="isImage || isVideo || isAudio"
        v-model="isOpen"
        max-w768px
        p8
        space-y-4
    >
        <h2 line-clamp-2 break-all>{{ data.fileName }}</h2>

        <div
            grid="~ gap4"
            :class="data.views ? 'sm:grid-cols-3' : 'sm:grid-cols-2'"
        >
            <div
                v-if="data.views"
                flex="~ items-center gap2"
                rounded-md
                bg-fs3
                p4
            >
                <Icon name="heroicons-solid:eye" text-fs-accent size="24" />
                <span font-medium="!">{{ data.views.today }} today</span>
            </div>
            <div flex="~ items-center gap2" rounded-md bg-fs3 p4>
                <Icon name="mdi:sd-storage" text-fs-accent size="24" />
                <span font-medium="!">{{ data.size!.formatted }}</span>
            </div>
            <div flex="~ items-center gap2" rounded-md bg-fs3 p4>
                <Icon
                    name="heroicons-solid:calendar"
                    text-fs-accent
                    size="24"
                />
                <span font-medium="!">
                    {{ dayjs(data.createdAt).fromNow() }}
                </span>
            </div>
        </div>

        <div
            v-if="isImage"
            h96
            wfull
            cursor-pointer
            select-none
            rounded-md
            bg-contain
            bg-center
            bg-no-repeat
            :style="{
                backgroundImage: `url(/u/${data.fileName})`,
            }"
            @click="
                (event) => (event.target as HTMLDivElement).requestFullscreen()
            "
        />

        <video
            v-else-if="isVideo"
            :src="`/u/${data.fileName}`"
            controls
            h96
            wfull
            rounded-md
        />

        <audio
            v-else-if="isAudio"
            :src="`/u/${data.fileName}`"
            controls
            wfull
        />

        <UiButton
            alignment="center"
            variant="accent"
            icon="heroicons-solid:x"
            icon-size="24"
            wfull
            gap2
            @click="isOpen = false"
        >
            Close
        </UiButton>
    </UiModal>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const isOpen = defineModel<boolean>({ required: true });

const { data } = defineProps<{
    data: Partial<FileData>;
}>();

const isImage = computed(() => data.mimeType!.startsWith('image/'));
const isVideo = computed(() => data.mimeType!.startsWith('video/'));
const isAudio = computed(() => data.mimeType!.startsWith('audio/'));
</script>
