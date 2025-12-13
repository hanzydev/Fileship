<template>
    <ModalsEditFile v-if="currentUser?.id === data.authorId" v-model="editModalOpen" :data />

    <UiDropdown v-model="ctxOpen" as-ctx-menu placement="bottom" trigger-class="hfull">
        <div
            relative
            rounded-xl
            motion-safe:transition-shadow
            ring="1 fs-overlay-4"
            :class="[
                selected && '!ring-2 !ring-fs-accent',
                !selectable && (ctxOpen || !canBeViewed)
                    ? 'cursor-default'
                    : 'cursor-pointer hover:(ring-1 ring-fs-accent)',
            ]"
            @click="selectable ? (selected = !selected) : canBeViewed && emit('viewFile', data)"
        >
            <Transition
                enter-active-class="motion-safe:(animate-in fade-in)"
                leave-active-class="motion-safe:(animate-out fade-out)"
            >
                <div
                    v-if="selected"
                    flex="~ items-center justify-center"
                    absolute
                    z10
                    hfull
                    wfull
                    cursor-pointer
                    rounded-xl
                    backdrop-blur-sm
                    bg="[color-mix(in_srgb,_var(--fs-accent),_transparent_90%)]"
                >
                    <Icon name="lucide:check" size="56" />
                </div>
            </Transition>
            <Icon
                v-if="isVideo && !selected"
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
            <div
                v-if="isImage || isVideo"
                relative
                h208px
                wfull
                flex="~ col items-center justify-center gap2"
                overflow-hidden
                rounded-xl
                bg-fs-overlay-2
                p4
                text-center
            >
                <img
                    v-if="isImage"
                    :src="data.directUrl"
                    :alt="data.fileName"
                    :class="!selectable && 'hover:scale-105'"
                    absolute
                    hfull
                    wfull
                    object-contain
                    motion-safe:transition-transform
                />
                <video
                    v-if="isVideo"
                    :src="data.directUrl"
                    :alt="data.fileName"
                    :class="!selectable && 'hover:scale-105'"
                    :poster="data.thumbnailUrl !== null ? data.thumbnailUrl : undefined"
                    absolute
                    hfull
                    wfull
                    object-contain
                    motion-safe:transition-transform
                />
            </div>
            <div v-else h208px wfull rounded-xl bg-fs-overlay-2 p8 space-y-8>
                <h5 line-clamp-1 break-words text-fs-muted-3>
                    {{ data.fileName }}
                </h5>

                <div flex="~ col justify-between gap2" text-fs-muted-2 font-medium>
                    <div flex="~ gap2 items-center">
                        <Icon name="solar:eye-bold" size="20" />
                        <span>{{ data.views.today }} today</span>
                    </div>

                    <div flex="~ gap2 items-center">
                        <Icon name="solar:diskette-bold" size="20" />
                        <span>{{ data.size.formatted }}</span>
                    </div>

                    <div flex="~ gap2 items-center">
                        <Icon name="solar:calendar-bold" size="20" />
                        <span>
                            {{ dayjs(data.createdAt).format('MMM D, YYYY h:mm A') }}
                        </span>
                    </div>
                </div>
            </div>
        </div>
        <template #content>
            <div w48 rounded-xl bg-fs-overlay-2 p1.5 space-y-1 ring="1 fs-overlay-4">
                <UiButton
                    v-if="!selectable || canBeViewed"
                    variant="onOverlay"
                    icon="solar:eye-bold"
                    icon-size="20"
                    wfull
                    gap2
                    :href="selectable ? undefined : data.embedUrl"
                    @click="
                        if (selectable) {
                            ctxOpen = false;
                            emit('viewFile', data);
                        }
                    "
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
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    variant="onOverlay"
                    icon="solar:pen-2-bold"
                    icon-size="20"
                    wfull
                    gap2
                    @click="
                        ctxOpen = false;
                        editModalOpen = true;
                    "
                >
                    Edit
                </UiButton>
                <UiButton
                    v-if="currentUser?.id === data.authorId && data.folderId && !selectable"
                    variant="onOverlay"
                    icon="solar:remove-folder-bold"
                    icon-size="20"
                    wfull
                    gap2
                    :disabled="updating"
                    @click="handleMoveFile(null)"
                >
                    Take Out
                </UiButton>
                <UiDropdown
                    v-if="currentUser?.id === data.authorId && !selectable && !data.folderId"
                    placement="right"
                    hover
                >
                    <UiButton
                        icon="solar:add-folder-bold"
                        icon-size="20"
                        wfull
                        gap2
                        variant="onOverlay"
                        :disabled="updating"
                    >
                        Add to Folder
                    </UiButton>
                    <template #content>
                        <div
                            h64
                            w64
                            overflow-y-auto
                            rounded-xl
                            bg-fs-overlay-2
                            p1.5
                            ring="1 fs-overlay-4"
                            space-y-2
                        >
                            <UiSearchBar
                                v-model="addToFolderSearchQuery"
                                v-model:loading="isSearchingFolders"
                                placeholder="Search folders..."
                                h10="!"
                                rounded-lg="!"
                                input-class="!bg-fs-overlay-3 !rounded-lg"
                            />

                            <p
                                v-if="!filteredFolders.length && !addToFolderSearchQuery"
                                mx4
                                translate-y-16
                                text-center
                                text-fs-muted-2
                            >
                                There are no folders to display.
                            </p>

                            <div space-y-1>
                                <UiButton
                                    v-if="
                                        addToFolderSearchQuery &&
                                        !filteredFolders.find(
                                            (f) => f.name === addToFolderSearchQuery.trim(),
                                        )
                                    "
                                    :disabled="updating"
                                    wfull
                                    break-all
                                    @click="handleCreateFolderWithFile"
                                >
                                    Create folder "{{ addToFolderSearchQuery }}"
                                </UiButton>

                                <UiButton
                                    v-for="(folder, index) in filteredFolders"
                                    :key="index"
                                    :disabled="updating"
                                    variant="onOverlay"
                                    wfull
                                    gap2
                                    break-all
                                    icon="solar:folder-bold"
                                    icon-size="20"
                                    @click="handleMoveFile(folder.id)"
                                >
                                    {{ folder.name }}
                                </UiButton>
                            </div>
                        </div>
                    </template>
                </UiDropdown>
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    variant="onOverlay"
                    icon="solar:trash-bin-minimalistic-bold"
                    icon-size="20"
                    wfull
                    gap2
                    text-red-500
                    :disabled="deleting"
                    @click="handleDelete"
                >
                    Delete
                </UiButton>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
import dayjs from 'dayjs';

const { data } = defineProps<{
    data: FileData;
    selectable?: boolean;
}>();

const selected = defineModel<boolean>('selected', {
    required: false,
    default: false,
});

const emit = defineEmits<{
    viewFile: [FileData];
}>();

const currentUser = useAuthUser();
const folders = useFolders();
const embed = useEmbed();
const { $toast } = useNuxtApp();

const addToFolderSearchQuery = ref('');
const searchedFolders = ref<string[]>([]);
const isSearchingFolders = ref(false);

const filteredFolders = computed(() =>
    folders.value.filter((f) =>
        !isSearchingFolders.value && addToFolderSearchQuery.value.length
            ? searchedFolders.value.includes(f.id)
            : true,
    ),
);

const isImage = computed(() => data.mimeType.startsWith('image/'));
const isVideo = computed(() => data.mimeType.startsWith('video/'));
const isAudio = computed(() => data.mimeType.startsWith('audio/'));

const canBeViewed = computed(() => isImage.value || isVideo.value || isAudio.value);

const editModalOpen = ref(false);

const ctxOpen = ref(false);
const deleting = ref(false);
const updating = ref(false);

const handleDelete = async () => {
    deleting.value = true;

    try {
        await $fetch(`/api/files/${data.id}`, { method: 'DELETE' });
        $toast.success('File deleted successfully');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    deleting.value = false;
};

const handleCopy = () => {
    useClipboard({ legacy: true }).copy(embed.value.enabled ? data.embedUrl : data.directUrl);
    ctxOpen.value = false;

    $toast.success('Link copied to clipboard');
};

const handleMoveFile = async (folderId: string | null) => {
    updating.value = true;

    try {
        await $fetch(`/api/files/${data.id}`, {
            method: 'PATCH',
            body: {
                folderId,
            },
        });
        $toast.success(
            folderId === null ? 'File removed from folder successfully' : 'File moved successfully',
        );
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    updating.value = false;
};

const handleCreateFolderWithFile = async () => {
    updating.value = true;

    try {
        await $fetch('/api/folders', {
            method: 'POST',
            body: {
                name: addToFolderSearchQuery.value.trim(),
                files: [data.id],
            },
        });

        $toast.success('Folder created and file added successfully');
    } catch (error: any) {
        if (error.data.data) $toast.error(error.data.data.name._errors[0]);
        else $toast.error(error.data.message);
    }

    updating.value = false;
};

let searchTimeout: NodeJS.Timeout;

watch(addToFolderSearchQuery, (query) => {
    clearTimeout(searchTimeout);

    if (query.length) {
        isSearchingFolders.value = true;

        searchTimeout = setTimeout(async () => {
            try {
                searchedFolders.value = await $fetch<string[]>('/api/folders/search', {
                    method: 'POST',
                    body: { query },
                });
            } catch {
                searchedFolders.value = [];
            }

            isSearchingFolders.value = false;
        }, 750);
    } else {
        searchedFolders.value = [];
    }
});
</script>
