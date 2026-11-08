<template>
    <div class="bgGrid overflow-x-clip">
        <div class="pt-16 lg:pt-24 w-full">
            <div
                class="container mx-auto flex flex-col items-center justify-center text-center gap-6 lg:gap-8"
            >
                <NuxtLink to="/docs" class="max-w-full">
                    <div
                        class="cursor-pointer rounded-3xl bg-card border flex p-1.5 justify-between w-fit max-w-full items-center gap-2 text-left hover:bg-secondary transition-colors duration-200"
                    >
                        <UiBadge>6.5</UiBadge>
                        <TextBlock variant="muted">
                            {{ $t('hero.release') }}
                        </TextBlock>
                        <IArrowRight class="size-4.5 shrink-0" />
                    </div>
                </NuxtLink>

                <TextBlock
                    variant="h1"
                    class="font-semibold! text-5xl! sm:text-6xl! lg:text-8xl! text-balance"
                >
                    <slot name="title" />
                </TextBlock>
                <TextBlock variant="muted" class="text-base! sm:text-lg! max-w-3xl">
                    <slot name="description" />
                </TextBlock>

                <div v-if="slots.actions" class="flex flex-wrap items-center justify-center gap-3">
                    <slot name="actions" />
                </div>

                <div class="flex flex-wrap items-center justify-center gap-1.5">
                    <TextBlock variant="muted">{{ $t('hero.license') }}</TextBlock>
                    <TextBlock variant="muted">{{ separator }}</TextBlock>
                    <TextBlock variant="muted">{{ dockerCompose }}</TextBlock>
                    <TextBlock variant="muted">{{ separator }}</TextBlock>
                    <TextBlock variant="muted">{{ $t('hero.languages') }}</TextBlock>
                </div>
            </div>

            <div class="mt-10 w-full">
                <div class="relative py-4 container mx-auto">
                    <NuxtImg
                        src="/dashboard.png"
                        :alt="$t('hero.dashboardAlt')"
                        class="select-none border-2 pb-2 sm:pb-4 bg-card rounded-t-2xl sm:rounded-t-4xl w-full shadow-[0px_0px_4rem_var(--primary)]"
                        draggable="false"
                    />
                    <NuxtImg
                        src="/mobile.png"
                        :alt="$t('hero.mobileAlt')"
                        class="absolute rounded-2xl sm:rounded-4xl border-4 sm:border-8 border-secondary bg-background right-0 -bottom-4 w-24 sm:-right-4 sm:-bottom-6 sm:w-44 lg:-right-8 lg:-bottom-8 lg:w-72 select-none shadow-[0px_0px_1rem_var(--background)]"
                        draggable="false"
                    />
                </div>

                <div
                    class="px-4 py-6 lg:p-8 flex flex-wrap items-center justify-center gap-x-8 gap-y-3 lg:gap-10 z-1 relative -mt-6 w-full bg-background border-y"
                >
                    <TextBlock
                        variant="muted"
                        class="uppercase basis-full text-center lg:basis-auto"
                    >
                        {{ $t('hero.uploadFrom') }}
                    </TextBlock>
                    <TextBlock v-for="item in uploaders" :key="item" variant="small">
                        {{ item }}
                    </TextBlock>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
const slots = useSlots();
const { t } = useI18n();

const dockerCompose = 'Docker Compose';
const separator = '・';

const uploaders = computed(() => [
    'ShareX',
    t('hero.windowsApp'),
    'macOS',
    'Linux',
    'Chrome & Edge',
    'Firefox',
    t('hero.androidShare'),
]);
</script>

<style scoped>
.bgGrid {
    background-image:
        linear-gradient(var(--border) 1px, transparent 1px),
        linear-gradient(90deg, var(--border) 1px, transparent 1px),
        linear-gradient(var(--border) 0.5px, transparent 0.5px),
        linear-gradient(90deg, var(--border) 0.5px, transparent 0.5px);
    background-size:
        4rem 4rem,
        4rem 4rem,
        10px 10px,
        10px 10px;
    background-position:
        -1px -1px,
        -1px -1px,
        -0.5px -0.5px,
        -0.5px -0.5px;
}
</style>
