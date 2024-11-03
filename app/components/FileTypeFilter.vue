<template>
    <UiDropdown placement="bottom" pt1.5="!">
        <UiButton
            icon="heroicons-solid:filter"
            icon-size="20"
            h12
            w36
            gap2
            text-slate300
            sm:w56
            ring-none="!"
            border="1 dashed fs-overlay-4 hover:(fs-accent solid)"
        >
            Filter type
        </UiButton>
        <template #content>
            <div
                max-h64
                w36
                overflow-y-auto
                rounded-lg
                bg-fs-overlay-2
                p1.5
                sm:w56
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
                    :variant="filterType.includes(_type as never) ? 'accent' : 'primary'"
                    icon-size="20"
                    wfull
                    gap2
                    @click="
                        () => {
                            const findIndex = filterType.indexOf(_type as never);
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
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

const filterType = defineModel<string[]>({ required: true });
</script>
