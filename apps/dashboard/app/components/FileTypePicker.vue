<template>
    <UiDropdown v-model="isOpen" placement="bottom" pt1.5="!">
        <slot />
        <template #content>
            <div
                relative
                h72
                w72
                overflow-y-auto
                rounded-xl
                bg-fs-overlay-2
                p1.5
                ring="1 fs-overlay-4"
                space-y-2
            >
                <UiSearchBar
                    v-model="searchQuery"
                    placeholder="Search file types..."
                    h10="!"
                    rounded-lg="!"
                    input-class="!bg-fs-overlay-3 !rounded-lg"
                />
                <div space-y-1>
                    <UiButton
                        v-for="(lang, index) in searched"
                        :key="index"
                        :icon="
                            lang.label === fileType.label
                                ? 'heroicons-solid:check'
                                : 'heroicons-solid:code'
                        "
                        :variant="lang.label === fileType.label ? 'accent' : 'onOverlay'"
                        icon-size="20"
                        wfull
                        gap2
                        @click="
                            fileType = lang;
                            isOpen = false;
                        "
                    >
                        {{ lang.label }}
                    </UiButton>
                </div>
            </div>
        </template>
    </UiDropdown>
</template>

<script setup lang="ts">
const isOpen = ref(false);

const fileType = defineModel<{
    label: string;
}>({
    required: true,
});

const searchQuery = ref('');

const searched = computed(() =>
    TEXT_FILE_TYPES.filter((t) =>
        t.label.toLowerCase().includes(searchQuery.value.toLowerCase()),
    ),
);
</script>
