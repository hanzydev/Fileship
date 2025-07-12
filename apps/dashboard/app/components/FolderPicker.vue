<template>
    <UiDropdown v-model="isOpen" placement="top" pb0.5="!">
        <slot />
        <template #content>
            <div
                relative
                top-6
                h64
                w56
                overflow-y-auto
                rounded-xl
                bg-fs-overlay-2
                p1.5
                space-y-2
                ring="1 fs-overlay-4"
            >
                <UiSearchBar
                    v-model="searchQuery"
                    v-model:loading="isSearching"
                    placeholder="Search folders..."
                    h10="!"
                    rounded-lg="!"
                    input-class="!bg-fs-overlay-3 !rounded-lg"
                />

                <p
                    v-if="!filtered.length && !searchQuery"
                    mx4
                    translate-y-16
                    text-center
                    text-fs-muted-2
                >
                    There are no folders to display.
                </p>

                <div space-y-1>
                    <UiButton
                        v-if="searchQuery && !filtered.find((f) => f.name === searchQuery.trim())"
                        wfull
                        break-all
                        :disabled="isCreating"
                        @click="handleCreate(searchQuery.trim())"
                    >
                        Create folder "{{ searchQuery }}"
                    </UiButton>

                    <UiButton
                        v-for="(option, index) in filtered"
                        :key="index"
                        :icon="
                            option.id === folder.value
                                ? 'heroicons-solid:check'
                                : 'heroicons-solid:folder'
                        "
                        :variant="option.id === folder.value ? 'accent' : 'onOverlay'"
                        icon-size="20"
                        wfull
                        gap2
                        break-all
                        :disabled="isCreating"
                        @click="
                            folder =
                                folder.value === option.id
                                    ? defaultValue
                                    : { label: option.name, value: option.id };
                            isOpen = false;
                        "
                    >
                        {{ option.name }}
                    </UiButton>
                </div>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
const folder = defineModel<{
    label: string;
    value: string | null;
}>({
    required: true,
});

const folders = useFolders();
const { $toast } = useNuxtApp();

const searchQuery = ref('');
const searched = ref<string[]>([]);

const isOpen = ref(false);
const isCreating = ref(false);
const isSearching = ref(false);

const defaultValue = {
    label: 'None',
    value: null,
};

const filtered = computed(() =>
    folders.value.filter((f) =>
        !isSearching.value && searchQuery.value.length ? searched.value.includes(f.id) : true,
    ),
);

const handleCreate = async (name: string) => {
    isCreating.value = true;

    try {
        const created = await $fetch('/api/folders', {
            method: 'POST',
            body: { name },
        });

        folder.value = { label: created.name, value: created.id };
        searchQuery.value = '';
        isOpen.value = false;

        $toast.success('Folder created and selected');
    } catch (error: any) {
        if (error.data.data) $toast.error(error.data.data.name._errors[0]);
        else $toast.error(error.data.message);
    }

    isCreating.value = false;
};

let searchTimeout: NodeJS.Timeout;

watch(searchQuery, (query) => {
    clearTimeout(searchTimeout);

    if (query.length) {
        isSearching.value = true;

        searchTimeout = setTimeout(async () => {
            try {
                searched.value = await $fetch<string[]>('/api/folders/search', {
                    method: 'POST',
                    body: { query },
                });
            } catch {
                searched.value = [];
            }

            isSearching.value = false;
        }, 750);
    } else {
        searched.value = [];
    }
});
</script>
