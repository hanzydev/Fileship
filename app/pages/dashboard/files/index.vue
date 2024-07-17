<template>
    <div>
        <Head>
            <Title>Files</Title>
        </Head>

        <div space-y-6>
            <h2>Files</h2>
            <div flex="~ gap4 1 items-center" wfull>
                <UiSearchBar
                    v-model="searchQuery"
                    placeholder="Search files..."
                    wfull
                />
                <UiDropdown placement="bottom">
                    <UiButton
                        icon="heroicons-solid:filter"
                        icon-size="20"
                        h12
                        w56
                        gap2
                        text-slate300
                    >
                        Filter type
                    </UiButton>
                    <template #content>
                        <div
                            max-h64
                            w56
                            overflow-y-auto
                            rounded-lg
                            bg-fs3
                            p1.5
                            space-y-1
                            ring="1 fs-accent"
                        >
                            <UiButton
                                v-for="_type in ['image', 'video', 'audio']"
                                :key="_type"
                                :icon="
                                    filterType.includes(_type as never)
                                        ? 'heroicons-solid:check'
                                        : 'heroicons-solid:filter'
                                "
                                :variant="
                                    filterType.includes(_type as never)
                                        ? 'accent'
                                        : 'primary'
                                "
                                icon-size="20"
                                wfull
                                gap2
                                @click="
                                    () => {
                                        const findIndex = filterType.indexOf(
                                            _type as never,
                                        );
                                        if (findIndex === -1) {
                                            filterType.push(_type as never);
                                        } else {
                                            filterType.splice(findIndex, 1);
                                        }
                                    }
                                "
                            >
                                {{ titleCase(_type) }}
                            </UiButton>
                        </div>
                    </template>
                </UiDropdown>
            </div>
            <div grid="~ gap6 lg:cols-3 md:cols-2 xl:cols-4">
                <New h208px @action="$router.push('/dashboard/files/upload')" />
                <FileCard
                    v-for="file in calculatedFiles"
                    :key="file.id"
                    :data="file"
                />
            </div>
            <UiPagination
                v-model="currentPage"
                :item-count="filtered.length"
                :items-per-page="19"
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

const files = useFiles();
const folders = useFolders();

const searchQuery = ref('');
const currentPage = ref(1);
const filterType = ref([]);

const { data: foldersData } = await useFetch('/api/folders');
const { data: filesData } = await useFetch('/api/files');

folders.value = foldersData.value!.map((f) => ({
    ...f,
    createdAt: new Date(f.createdAt),
}));

files.value = filesData.value!.map((f) => ({
    ...f,
    expiresAt: f.expiresAt ? new Date(f.expiresAt) : null,
    createdAt: new Date(f.createdAt),
}));

const filtered = computed(() =>
    files.value
        .filter((f) => !f.folderId)
        .filter(
            (f) =>
                !filterType.value.length ||
                filterType.value.some((t) => f.mimeType.startsWith(`${t}/`)),
        )
        .filter((f) =>
            Object.values(f).some((v) =>
                v
                    ?.toString()
                    ?.toLowerCase()
                    ?.includes(searchQuery.value.toLowerCase()),
            ),
        ),
);

const calculatedFiles = computed(() => {
    const start = (currentPage.value - 1) * 19;
    const end = start + 19;
    return filtered.value.slice(start, end);
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'only-file-uploader',
});
</script>
