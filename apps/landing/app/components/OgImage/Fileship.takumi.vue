<template>
    <div
        class="relative flex h-full w-full flex-col justify-between overflow-hidden px-[72px] py-[64px]"
        style="background-color: #09090b; font-family: Geist"
    >
        <div
            class="absolute"
            style="
                top: -320px;
                right: -220px;
                width: 900px;
                height: 900px;
                background-image: radial-gradient(
                    circle,
                    rgba(99, 102, 241, 0.55) 0%,
                    rgba(79, 70, 229, 0.18) 38%,
                    rgba(9, 9, 11, 0) 70%
                );
            "
        />
        <div
            class="absolute"
            style="
                inset: 0;
                background-image:
                    linear-gradient(rgba(255, 255, 255, 0.04) 1px, transparent 1px),
                    linear-gradient(90deg, rgba(255, 255, 255, 0.04) 1px, transparent 1px);
                background-size: 48px 48px;
            "
        />

        <div class="relative flex items-center" style="gap: 20px">
            <img src="/fileship.png" width="64" height="64" />
            <span style="color: #fafafa; font-size: 40px; font-weight: 700">{{ brand }}</span>
        </div>

        <div class="relative flex flex-col" style="gap: 24px; max-width: 960px">
            <span style="color: #a5b4fc; font-size: 28px; font-weight: 600">{{ headline }}</span>
            <span
                style="
                    color: #fafafa;
                    font-size: 68px;
                    font-weight: 700;
                    line-height: 1.1;
                    letter-spacing: -0.02em;
                    line-clamp: 2;
                    text-overflow: ellipsis;
                "
            >
                {{ title }}
            </span>
            <span
                v-if="description"
                style="
                    color: #a1a1aa;
                    font-size: 30px;
                    line-height: 1.4;
                    line-clamp: 2;
                    text-overflow: ellipsis;
                "
            >
                {{ description }}
            </span>
        </div>

        <div class="relative flex items-center justify-between">
            <span style="color: #71717a; font-size: 24px">{{ tagline }}</span>
            <span style="color: #d4d4d8; font-size: 26px; font-weight: 600">{{ host }}</span>
        </div>
    </div>
</template>

<script setup lang="ts">
const {
    title = 'Fileship',
    description = '',
    headline = 'Fileship',
    tagline = 'Open source · Self-hosted',
} = defineProps<{
    title?: string;
    description?: string;
    headline?: string;
    tagline?: string;
}>();

const brand = 'Fileship';
const host = 'fileship.app';
</script>
