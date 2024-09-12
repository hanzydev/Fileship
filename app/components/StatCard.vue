<template>
    <component
        :is="loading ? UiSkeletonCard : 'div'"
        flex="~ col justify-between"
        h124px
        rounded-md
        bg-fs-overlay-2
        p4
        space-y-2
    >
        <div flex="~ items-center justify-between">
            <h6 text-slate400 uppercase>{{ title }}</h6>
            <Icon :name="icon" size="24" text-fs-accent />
        </div>
        <div flex="~ items-baseline gap2">
            <UiSkeletonLine
                v-if="loading"
                h6
                :style="{
                    width: `${randomNumber(20, 40)}%`,
                }"
            />
            <h4 v-else>{{ data }}</h4>

            <div
                v-if="growth !== 0 && !loading"
                flex="~ gap0.5 items-center"
                text-sm="!"
                font-medium="!"
                :class="growth! > 0 ? 'text-green500' : 'text-red-500'"
            >
                {{ growth }}%
                <Icon
                    :name="
                        growth! > 0
                            ? 'heroicons:arrow-up-right-16-solid'
                            : 'heroicons:arrow-down-right-16-solid'
                    "
                    size="20"
                />
            </div>
        </div>
        <div text-slate300>{{ description }}</div>
    </component>
</template>

<script setup lang="ts">
import { UiSkeletonCard } from '#components';

defineProps<{
    title: string;
    description: string;
    icon: string;
    growth?: number;
    data: any;
    loading?: boolean;
}>();
</script>
