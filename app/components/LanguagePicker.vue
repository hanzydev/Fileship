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
                rounded-lg
                bg-fs-overlay-2
                p1.5
                ring="1 fs-accent"
                space-y-2
            >
                <UiSearchBar
                    v-model="searchQuery"
                    placeholder="Search languages..."
                    input-class="!h10 !bg-fs-overlay-3 !ring-0"
                />
                <div space-y-1>
                    <UiButton
                        v-for="(lang, index) in results.map((r) => r.item)"
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
import { useFuse } from '@vueuse/integrations/useFuse';

const isOpen = ref(false);

const language = defineModel<{
    label: string;
    hljs: string;
}>({
    required: true,
});

const searchQuery = ref('');

const { results } = useFuse(searchQuery, languages, {
    matchAllWhenSearchEmpty: true,
    fuseOptions: {
        keys: ['label'],
    },
});
</script>
