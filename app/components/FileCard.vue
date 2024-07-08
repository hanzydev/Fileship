<template>
    <ModalsViewFile v-if="canBeViewed" v-model="viewModalOpen" :data />

    <ModalsEditFile
        v-if="currentUser?.id === data.authorId"
        v-model="editModalOpen"
        :data
    />

    <UiDropdown
        v-model="ctxOpen"
        as-ctx-menu
        placement="bottom"
        trigger-class="hfull"
    >
        <div
            v-if="isImage || isVideo"
            wfull
            rounded-md
            bg-fs3
            motion-safe:transition-shadow
            :class="[
                selected && 'ring-2 ring-fs-accent',
                ctxOpen
                    ? 'cursor-default'
                    : 'cursor-pointer hover:(ring-2 ring-fs-accent)',
            ]"
            @click="
                selectable ? (selected = !selected) : (viewModalOpen = true)
            "
        >
            <div
                v-if="isImage || isVideo"
                relative
                h208px
                flex="~ col items-center justify-center gap2"
                overflow-hidden
                rounded-md
                p4
                text-center
            >
                <img
                    v-if="isImage"
                    :src="`/u/${data.fileName}`"
                    :alt="data.fileName"
                    absolute
                    hfull
                    wfull
                    object-contain
                    hover:scale-105
                    motion-safe:transition-transform
                />
                <video
                    v-if="isVideo"
                    :src="`/u/${data.fileName}`"
                    :alt="data.fileName"
                    absolute
                    hfull
                    wfull
                    object-contain
                    hover:scale-105
                    motion-safe:transition-transform
                />
            </div>
        </div>
        <div
            v-else
            h208px
            wfull
            rounded-md
            bg-fs3
            p8
            space-y-8
            motion-safe:transition-shadow
            :class="[
                {
                    'hover:(ring-2 ring-fs-accent)':
                        (selectable || isAudio) && !ctxOpen,
                    'cursor-default': ctxOpen,
                    'ring-2 ring-fs-accent': selected,
                    'cursor-pointer': selectable || (isAudio && !ctxOpen),
                },
            ]"
            @click="
                selectable
                    ? (selected = !selected)
                    : isAudio && (viewModalOpen = true)
            "
        >
            <h5 line-clamp-1 break-words text-slate400>
                {{ data.fileName }}
            </h5>

            <div flex="~ col justify-between gap2" text-slate-300 font-medium>
                <div flex="~ gap2 items-center">
                    <Icon name="heroicons-solid:eye" size="20" />
                    <span>{{ data.views.today }} today</span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon name="mdi:sd-storage" size="20" />
                    <span>{{ data.size.formatted }}</span>
                </div>

                <div flex="~ gap2 items-center">
                    <Icon name="heroicons-solid:calendar" size="20" />
                    <span>
                        {{
                            moment(data.createdAt).format('MMM D, YYYY h:mm A')
                        }}
                    </span>
                </div>
            </div>
        </div>
        <template #content>
            <div w48 rounded-lg bg-fs3 p1.5 space-y-1 ring="2 fs-accent">
                <UiButton
                    v-if="selectable ? canBeViewed : true"
                    icon="heroicons:eye-16-solid"
                    icon-size="20"
                    wfull
                    gap2
                    :href="selectable ? undefined : `/view/${data.fileName}`"
                    @click="
                        if (selectable) {
                            ctxOpen = false;
                            viewModalOpen = true;
                        }
                    "
                >
                    Open
                </UiButton>
                <UiButton
                    icon="heroicons-solid:clipboard-copy"
                    icon-size="20"
                    wfull
                    gap2
                    @click="handleCopy"
                >
                    Copy Link
                </UiButton>
                <UiButton
                    icon="heroicons-solid:download"
                    icon-size="20"
                    wfull
                    gap2
                    :href="`/u/${data.fileName}?download`"
                    target="_blank"
                >
                    Download
                </UiButton>
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    icon="heroicons:pencil-16-solid"
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
                    v-if="
                        currentUser?.id === data.authorId &&
                        data.folderId &&
                        !selectable
                    "
                    icon="heroicons-solid:folder-remove"
                    icon-size="20"
                    wfull
                    gap2
                    :disabled="updating"
                    @click="changeFolder(null)"
                >
                    Take Out
                </UiButton>
                <UiDropdown
                    v-if="
                        currentUser?.id === data.authorId &&
                        !selectable &&
                        !data.folderId
                    "
                    :placement="width < 768 ? 'bottom' : 'right'"
                    hover
                    md:pl4
                    lt-md:-left-10
                >
                    <UiButton
                        icon="heroicons-solid:folder-add"
                        icon-size="20"
                        wfull
                        gap2
                        :disabled="updating"
                    >
                        Add to Folder
                    </UiButton>
                    <template #content>
                        <div
                            h64
                            w64
                            overflow-y-auto
                            rounded-lg
                            bg-fs3
                            p1.5
                            ring="2 fs-accent"
                            space-y-2
                        >
                            <UiSearchBar
                                v-model="addToFolderSearchQuery"
                                placeholder="Search folders..."
                                h-10="!"
                                bg-fs2="!"
                                ring-0="!"
                            />

                            <p
                                v-if="
                                    !filteredFolders.length &&
                                    !addToFolderSearchQuery
                                "
                                mx4
                                translate-y-16
                                text-center
                                text-slate300
                            >
                                There are no folders to display.
                            </p>

                            <div space-y-1>
                                <UiButton
                                    v-if="addToFolderSearchQuery"
                                    :disabled="updating"
                                    wfull
                                    break-all
                                    @click="createFolderAndAdd"
                                >
                                    Create folder "{{ addToFolderSearchQuery }}"
                                </UiButton>

                                <UiButton
                                    v-for="(folder, index) in filteredFolders"
                                    :key="index"
                                    :disabled="updating"
                                    wfull
                                    gap2
                                    @click="changeFolder(folder.id)"
                                >
                                    {{ folder.name }}
                                </UiButton>
                            </div>
                        </div>
                    </template>
                </UiDropdown>
                <UiButton
                    v-if="currentUser?.id === data.authorId"
                    icon="heroicons-solid:trash"
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
import moment from 'moment';
import { toast } from 'vue-sonner';

const { data } = defineProps<{
    data: FileData;
    selectable?: boolean;
}>();

const selected = defineModel<boolean>('selected', {
    required: false,
    default: false,
});

const { width } = useWindowSize();

const currentUser = useAuthUser();
const folders = useFolders();

const addToFolderSearchQuery = ref('');

const filteredFolders = computed(() =>
    folders.value.filter((l) =>
        Object.values(l).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(addToFolderSearchQuery.value.toLowerCase()),
        ),
    ),
);

const isImage = computed(() => data.mimeType.startsWith('image/'));
const isVideo = computed(() => data.mimeType.startsWith('video/'));
const isAudio = computed(() => data.mimeType.startsWith('audio/'));

const canBeViewed = computed(
    () => isImage.value || isVideo.value || isAudio.value,
);

const viewModalOpen = ref(false);
const editModalOpen = ref(false);

const ctxOpen = ref(false);
const deleting = ref(false);
const updating = ref(false);

const handleDelete = async () => {
    deleting.value = true;
    await $fetch(`/api/files/${data.id}`, { method: 'DELETE' });
    deleting.value = false;

    toast.success('File deleted successfully');
};

const handleCopy = () => {
    navigator.clipboard.writeText(
        `${useRequestURL().origin}/${currentUser.value!.embed?.enabled ? 'view' : 'u'}/${data.fileName}`,
    );
    ctxOpen.value = false;

    toast.success('Link copied to clipboard');
};

const changeFolder = async (folderId: string | null) => {
    updating.value = true;

    await $fetch(`/api/files/${data!.id}`, {
        method: 'PATCH',
        body: {
            folderId,
        },
    });

    updating.value = false;

    toast.success('File moved successfully');
};

const createFolderAndAdd = async () => {
    updating.value = true;

    try {
        await $fetch('/api/folders', {
            method: 'POST',
            body: {
                name: addToFolderSearchQuery.value,
                public: false,
                files: [data.id],
            },
        });

        toast.success('Folder created and file added successfully');
    } catch (error: any) {
        if (error.data.data) toast.error(error.data.data.name._errors[0]);
        else toast.error(error.data.message);
    }

    updating.value = false;
};
</script>
