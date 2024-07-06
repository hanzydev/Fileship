<template>
    <UiDropdown v-model="isOpen" placement="top">
        <slot />
        <template #content>
            <div
                relative
                top-6
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
                    v-model="searchQuery"
                    placeholder="Search languages..."
                    h-10="!"
                    bg-fs2="!"
                    ring-0="!"
                />
                <div space-y-1>
                    <UiButton
                        v-for="(lang, index) in filtered"
                        :key="index"
                        icon="heroicons-solid:code"
                        :variant="
                            lang.label === language.label ? 'accent' : 'primary'
                        "
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

const filtered = computed(() =>
    languages.filter((l) =>
        Object.values(l).some((v) =>
            v
                ?.toString()
                ?.toLowerCase()
                ?.includes(searchQuery.value.toLowerCase()),
        ),
    ),
);
</script>
