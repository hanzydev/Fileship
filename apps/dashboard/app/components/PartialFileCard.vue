<template>
    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom" trigger-class="hfull">
        <div
            v-if="isImage || isVideo"
            wfull
            rounded-xl
            bg-fs-overlay-2
            motion-safe:transition-shadow
            ring="1 fs-overlay-4"
            :class="ctxOpen ? 'cursor-default' : 'cursor-pointer hover:(ring-1 ring-fs-accent)'"
            @click="emit('viewFile', data)"
        >
            <div
                relative
                h52
                overflow-hidden
                rounded-xl
                p4
                text-center
                flex="~ col items-center justify-center gap2"
            >
                <img
                    v-if="isImage"
                    :src="data.directUrl"
                    :alt="data.fileName"
                    absolute
                    hfull
                    wfull
                    object-contain
                    hover:scale-105
                    motion-safe:transition-transform
                />

                <template v-if="isVideo">
                    <Icon
                        name="solar:play-bold"
                        size="64"
                        absolute
                        z10
                        top="1/2"
                        left="1/2"
                        translate-y-="1/2"
                        translate-x-="1/2"
                        op75
                    />
                    <video
                        v-if="isVideo"
                        :src="data.directUrl"
                        :alt="data.fileName"
                        :poster="data.thumbnailUrl !== null ? data.thumbnailUrl : undefined"
                        absolute
                        hfull
                        wfull
                        object-contain
                        hover:scale-105
                        motion-safe:transition-transform
                    />
                </template>
            </div>
        </div>
        <div
            v-else
            h52
            wfull
            rounded-xl
            bg-fs-overlay-2
            p8
            space-y-8
            motion-safe:transition-shadow
            :class="{
                'hover:(ring-1 ring-fs-accent)': isAudio && !ctxOpen,
                'cursor-default': ctxOpen,
                'cursor-pointer': isAudio && !ctxOpen,
            }"
            @click="isAudio && (viewModalOpen = true)"
        >
            <h5 line-clamp-1 break-words text-fs-muted-3>
                {{ data.fileName }}
            </h5>

            <div flex="~ col justify-between gap2" justify-between text-fs-muted-2 font-medium="!">
                <div flex="~ gap2 items-center">
                    <Icon name="solar:diskette-bold" size="20" />
                    <span>{{ data.size!.formatted }}</span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon name="solar:calendar-bold" size="20" />
                    <span>
                        {{ dayjs(data.createdAt).format('MMM D, YYYY h:mm A') }}
                    </span>
                </div>
            </div>
        </div>
        <template #content>
            <div w48 rounded-xl bg-fs-overlay-2 p1.5 space-y-1 ring="1 fs-overlay-4">
                <UiButton
                    variant="onOverlay"
                    icon="solar:eye-bold"
                    icon-size="20"
                    wfull
                    gap2
                    :href="data.embedUrl"
                >
                    Open
                </UiButton>
                <UiButton
                    variant="onOverlay"
                    icon="solar:clipboard-bold"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy Link
                </UiButton>
                <UiButton
                    variant="onOverlay"
                    icon="solar:download-minimalistic-bold"
                    icon-size="20"
                    wfull
                    gap2
                    :href="`${data.directUrl}?download`"
                    target="_blank"
                >
                    Download
                </UiButton>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const { data } = defineProps<{
    data: Partial<FileData> & { embed: IEmbed };
}>();

const emit = defineEmits<{
    viewFile: [Partial<FileData> & { embed: IEmbed }];
}>();

const { $toast } = useNuxtApp();

const isImage = computed(() => data.mimeType!.startsWith('image/'));
const isVideo = computed(() => data.mimeType!.startsWith('video/'));
const isAudio = computed(() => data.mimeType!.startsWith('audio/'));

const viewModalOpen = ref(false);
const ctxOpen = ref(false);

const handleCopy = () => {
    useClipboard({ legacy: true }).copy(data.embed.enabled ? data.embedUrl! : data.directUrl!);
    ctxOpen.value = false;

    $toast.success('Link copied to clipboard');
};
</script>
