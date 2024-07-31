<template>
    <div rounded-md bg-fs-overlay-2 p4 space-y-2>
        <div flex="~ items-center justify-between">
            <h6 text-slate400 uppercase>{{ title }}</h6>
            <Icon :name="icon" size="24" text-fs-accent />
        </div>
        <div flex="~ items-baseline gap2">
            <h4>{{ data }}</h4>
            <div
                v-if="growth !== 0"
                flex="~ gap2 items-center"
                text-sm="!"
                font-medium="!"
                :class="growth > 0 ? 'text-green500' : 'text-red-500'"
            >
                {{ growth }}%
                <Icon
                    :name="
                        growth > 0
                            ? 'heroicons:arrow-up-right-16-solid'
                            : 'heroicons:arrow-down-right-16-solid'
                    "
                    size="20"
                />
            </div>
        </div>
        <div text-slate300>{{ description }}</div>
    </div>
</template>

<script setup lang="ts">
defineProps<{
    title: string;
    description: string;
    icon: string;
    growth: number;
    data: any;
}>();
</script>
