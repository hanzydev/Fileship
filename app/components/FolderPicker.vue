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
                rounded-lg
                bg-fs-overlay-2
                p1.5
                space-y-2
                ring="1 fs-accent"
            >
                <UiSearchBar
                    v-model="searchQuery"
                    placeholder="Search folders..."
                    input-class="!h10 !bg-fs-overlay-3 !ring-0"
                />

                <p
                    v-if="!folders.length && !searchQuery"
                    mx4
                    translate-y-16
                    text-center
                    text-slate300
                >
                    There are no folders to display.
                </p>

                <div space-y-1>
                    <UiButton
                        v-if="
                            searchQuery && !results.find((r) => r.item.name === searchQuery.trim())
                        "
                        wfull
                        break-all
                        :disabled="isCreating"
                        @click="handleCreate(searchQuery.trim())"
                    >
                        Create folder "{{ searchQuery }}"
                    </UiButton>

                    <UiButton
                        v-for="(option, index) in folders"
                        :key="index"
                        :icon="
                            option.id === folder.value
                                ? 'heroicons-solid:check'
                                : 'heroicons-solid:folder'
                        "
                        :variant="option.id === folder.value ? 'accent' : 'primary'"
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
import { toast } from 'vue-sonner';

import { useFuse } from '@vueuse/integrations/useFuse';

const folder = defineModel<{
    label: string;
    value: string | null;
}>({
    required: true,
});

const folders = useFolders();

const searchQuery = ref('');
const isOpen = ref(false);
const isCreating = ref(false);

const defaultValue = {
    label: 'None',
    value: null,
};

const { results } = useFuse(searchQuery, folders, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: ['name'],
    },
});

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

        toast.success('Folder created and selected');
    } catch (error: any) {
        if (error.data.data) toast.error(error.data.data.name._errors[0]);
        else toast.error(error.data.message);
    }

    isCreating.value = false;
};
</script>
