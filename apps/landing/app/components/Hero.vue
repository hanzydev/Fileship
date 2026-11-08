<template>
    <div ref="root" class="relative isolate overflow-x-clip">
        <svg
            aria-hidden="true"
            class="pointer-events-none absolute inset-0 -z-10 size-full text-border"
            shape-rendering="crispEdges"
        >
            <defs>
                <pattern id="heroGrid" width="64" height="64" patternUnits="userSpaceOnUse">
                    <path
                        :d="fineLines"
                        fill="none"
                        stroke="currentColor"
                        stroke-opacity="0.3"
                        stroke-width="1"
                    />
                    <path
                        d="M0 0.5H64M0.5 0V64"
                        fill="none"
                        stroke="currentColor"
                        stroke-width="1"
                    />
                </pattern>
            </defs>
            <rect width="100%" height="100%" fill="url(#heroGrid)" />
        </svg>
        <div class="pt-16 lg:pt-24 w-full">
            <div
                class="container mx-auto flex flex-col items-center justify-center text-center gap-6 lg:gap-8"
            >
                <NuxtLink
                    :to="latestRelease?.path ?? '/releases'"
                    class="max-w-full"
                    data-reveal
                    data-hero="text"
                >
                    <div
                        class="cursor-pointer rounded-3xl bg-card border flex p-1.5 justify-between w-fit max-w-full items-center gap-2 text-left hover:bg-secondary transition-colors duration-200"
                    >
                        <UiBadge v-if="latestVersion">{{ latestVersion }}</UiBadge>
                        <TextBlock variant="muted">
                            {{ $t('hero.release') }}
                        </TextBlock>
                        <IArrowRight class="size-4.5 shrink-0" />
                    </div>
                </NuxtLink>

                <TextBlock
                    data-reveal
                    data-hero="text"
                    variant="h1"
                    class="font-semibold! text-5xl! sm:text-6xl! lg:text-8xl! text-balance"
                >
                    <slot name="title" />
                </TextBlock>
                <TextBlock
                    data-reveal
                    data-hero="text"
                    variant="muted"
                    class="text-base! sm:text-lg! max-w-3xl"
                >
                    <slot name="description" />
                </TextBlock>

                <div
                    v-if="slots.actions"
                    data-reveal
                    data-hero="text"
                    class="flex flex-wrap items-center justify-center gap-3"
                >
                    <slot name="actions" />
                </div>

                <div
                    data-reveal
                    data-hero="text"
                    class="flex flex-wrap items-center justify-center gap-1.5"
                >
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
                        data-reveal
                        data-hero="dashboard"
                        src="/dashboard.png"
                        :alt="$t('hero.dashboardAlt')"
                        class="select-none border-2 pb-2 sm:pb-4 bg-card rounded-t-2xl sm:rounded-t-4xl w-full shadow-[0px_0px_4rem_var(--primary)]"
                        draggable="false"
                    />
                    <NuxtImg
                        data-reveal
                        data-hero="mobile"
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
import { gsap } from 'gsap';

const slots = useSlots();
const { t } = useI18n();

const { data: latestRelease } = await useAsyncData('latest-release', () =>
    queryCollection('releases').order('order', 'DESC').select('path', 'version').first(),
);

const latestVersion = computed(() => latestRelease.value?.version.split('.').slice(0, 2).join('.'));

const root = useTemplateRef<HTMLElement>('root');

onMounted(() => {
    const items = (name: string) =>
        root.value!.querySelectorAll<HTMLElement>(`[data-hero="${name}"]`);
    const all = root.value!.querySelectorAll<HTMLElement>('[data-hero]');
    const done = () => {
        all.forEach((element) => (element.dataset.revealDone = ''));
        gsap.set(all, { clearProps: 'opacity,transform' });
    };

    if (prefersReducedMotion()) {
        done();
        return;
    }

    gsap.timeline({ defaults: { ease: 'power2.out' }, onComplete: done })
        .to(items('text'), { opacity: 1, y: 0, duration: 0.5, stagger: 0.08 })
        .fromTo(
            items('dashboard'),
            { y: 24, scale: 0.98 },
            { opacity: 1, y: 0, scale: 1, duration: 0.8 },
            '-=0.2',
        )
        .fromTo(items('mobile'), { y: 40 }, { opacity: 1, y: 0, duration: 0.7 }, '-=0.65');
});

const fineLines = [8, 16, 24, 32, 40, 48, 56]
    .map((p) => `M0 ${p + 0.5}H64M${p + 0.5} 0V64`)
    .join('');

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
