<template>
    <UiDropdown v-model="isOpen" placement="top" pb0.5="!">
        <slot />
        <template #content>
            <div
                relative
                top-6
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
                    v-model="searchQuery"
                    placeholder="Search languages..."
                    h10="!"
                    rounded-lg="!"
                    input-class="!bg-fs-overlay-3 !rounded-lg"
                />
                <div space-y-1>
                    <UiButton
                        v-for="(lang, index) in searched"
                        :key="index"
                        :icon="
                            lang.label === language.label
                                ? 'heroicons-solid:check'
                                : 'heroicons-solid:code'
                        "
                        :variant="lang.label === language.label ? 'accent' : 'onOverlay'"
                        icon-size="20"
                        wfull
                        gap2
                        @click="
                            language = lang;
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

const language = defineModel<{
    label: string;
    hljs: string;
}>({
    required: true,
});

const searchQuery = ref('');

const searched = computed(() =>
    languages.filter((lang) => lang.label.toLowerCase().includes(searchQuery.value.toLowerCase())),
);
</script>
