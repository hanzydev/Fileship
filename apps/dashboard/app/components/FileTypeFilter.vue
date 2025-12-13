<template>
    <UiDropdown placement="bottom" wrapper-class="wfull sm:w56" pt1.5="!" wfull sm:w56>
        <UiButton
            icon="solar:filter-bold"
            icon-size="20"
            h12
            wfull
            gap2
            rounded-xl
            text-fs-muted-2
            sm:w56
            ring-none="!"
            border="1 dashed fs-overlay-4 hover:(fs-accent solid)"
            v-bind="$attrs"
        >
            Filter type
        </UiButton>
        <template #content>
            <div
                wfull
                overflow-y-auto
                rounded-xl
                bg-fs-overlay-2
                p1
                sm:w56
                space-y-1
                ring="1 fs-overlay-4"
                :class="dropdownClass"
            >
                <UiButton
                    v-for="_type in ['image', 'video', 'audio', 'document', 'archive', 'code']"
                    :key="_type"
                    :icon="
                        filterType.includes(_type as never) ? 'lucide:check' : 'solar:filter-bold'
                    "
                    :variant="filterType.includes(_type as never) ? 'accent' : 'onOverlay'"
                    icon-size="20"
                    wfull
                    gap2
                    :class="buttonClass"
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

const { dropdownClass, buttonClass } = defineProps<{
    dropdownClass?: unknown;
    buttonClass?: unknown;
}>();
</script>
