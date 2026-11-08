<template>
    <section id="features" class="scroll-mt-20 py-20 lg:py-28">
        <div class="container mx-auto flex flex-col gap-10 lg:gap-14">
            <SectionHeading>
                <template #eyebrow>{{ $t('features.eyebrow') }}</template>
                <template #title>{{ $t('features.title') }}</template>
                <template #description>{{ $t('features.description') }}</template>
            </SectionHeading>

            <div v-reveal.stagger class="grid grid-cols-1 gap-4 lg:grid-cols-6">
                <UiCard id="windowsApp" class="scroll-mt-24 lg:col-span-4">
                    <UiCardHeader class="max-w-xl gap-2">
                        <div class="flex items-center gap-2 text-chart-1">
                            <IScan class="size-5" />
                            <TextBlock variant="small">
                                {{ $t('features.windows.label') }}
                            </TextBlock>
                        </div>
                        <UiCardTitle class="text-2xl font-semibold">
                            {{ $t('features.windows.title') }}
                        </UiCardTitle>
                        <UiCardDescription class="text-base leading-relaxed">
                            {{ $t('features.windows.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent>
                        <div
                            class="relative h-72 overflow-hidden rounded-3xl border bg-muted/40 bg-[radial-gradient(var(--border)_1px,transparent_1px)] bg-size-[18px_18px]"
                        >
                            <div
                                class="absolute top-4 left-1/2 flex -translate-x-1/2 items-center gap-1 rounded-3xl border bg-background p-1.5 shadow-md"
                            >
                                <span
                                    v-for="(tool, index) in tools"
                                    :key="index"
                                    :class="
                                        cn(
                                            'flex size-8 items-center justify-center rounded-2xl text-muted-foreground',
                                            index === 0 && 'bg-muted text-foreground',
                                        )
                                    "
                                >
                                    <component :is="tool" class="size-4" />
                                </span>
                                <UiSeparator orientation="vertical" class="mx-1 h-5!" />
                                <span
                                    class="flex size-8 items-center justify-center rounded-2xl bg-destructive/20"
                                >
                                    <span class="size-2.5 rounded-full bg-destructive" />
                                </span>
                            </div>

                            <div
                                class="absolute top-24 left-[12%] h-36 w-[46%] border-[1.5px] border-chart-2 bg-chart-2/10"
                            >
                                <UiBadge class="absolute -top-6 -left-px font-mono">
                                    1280 × 720
                                </UiBadge>
                                <span
                                    class="absolute top-6 left-6 h-11 w-28 rounded-xl border-2 border-chart-1"
                                />
                                <span
                                    class="absolute top-14 left-40 font-semibold whitespace-nowrap text-chart-1"
                                >
                                    {{ $t('features.windows.annotation') }}
                                </span>
                            </div>

                            <div
                                class="absolute right-4 bottom-4 left-4 flex items-center gap-3 rounded-3xl border bg-background p-3 shadow-md sm:left-auto sm:w-72"
                            >
                                <span
                                    class="flex size-9 shrink-0 items-center justify-center rounded-full bg-primary text-primary-foreground"
                                >
                                    <ICheck class="size-4" />
                                </span>
                                <div class="flex min-w-0 flex-col gap-0.5">
                                    <TextBlock variant="small">
                                        {{ $t('features.windows.linkCopied') }}
                                    </TextBlock>
                                    <TextBlock variant="muted" class="truncate font-mono text-xs">
                                        {{ copiedLink }}
                                    </TextBlock>
                                </div>
                            </div>
                        </div>
                    </UiCardContent>
                </UiCard>

                <UiCard class="lg:col-span-2">
                    <UiCardHeader class="gap-2">
                        <div class="flex items-center gap-2 text-chart-1">
                            <ICloud class="size-5" />
                            <TextBlock variant="small">{{ harborName }}</TextBlock>
                        </div>
                        <UiCardTitle class="text-xl font-semibold">
                            {{ $t('features.harbor.title') }}
                        </UiCardTitle>
                        <UiCardDescription class="text-base leading-relaxed">
                            {{ $t('features.harbor.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent class="mt-auto">
                        <div class="overflow-hidden rounded-3xl border bg-muted/40 text-sm">
                            <div
                                class="flex justify-between border-b px-4 py-2.5 text-xs text-muted-foreground"
                            >
                                <span>{{ $t('features.harbor.name') }}</span>
                                <span>{{ $t('features.harbor.status') }}</span>
                            </div>
                            <div
                                v-for="item in harborItems"
                                :key="item.name"
                                :class="
                                    cn(
                                        'flex items-center gap-3 px-4 py-2.5',
                                        item.online && 'bg-primary/15',
                                    )
                                "
                            >
                                <component
                                    :is="item.icon"
                                    class="size-4 shrink-0 text-muted-foreground"
                                />
                                <span class="flex-1 truncate">{{ item.name }}</span>
                                <ICloud v-if="item.online" class="size-4 text-chart-1" />
                                <ICircleCheck v-else class="size-4 text-chart-1" />
                            </div>
                        </div>
                    </UiCardContent>
                </UiCard>

                <UiCard class="lg:col-span-3">
                    <UiCardHeader class="gap-2">
                        <div class="flex items-center gap-2 text-chart-1">
                            <ISparkles class="size-5" />
                            <TextBlock variant="small">{{ $t('features.ai.label') }}</TextBlock>
                        </div>
                        <UiCardTitle class="text-xl font-semibold">
                            {{ $t('features.ai.title') }}
                        </UiCardTitle>
                        <UiCardDescription class="text-base leading-relaxed">
                            {{ $t('features.ai.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent class="mt-auto flex flex-col gap-3">
                        <div
                            class="flex h-11 items-center gap-2.5 rounded-3xl border border-chart-2/60 bg-muted/40 px-4 text-sm"
                        >
                            <ISearch class="size-4 text-muted-foreground" />
                            <span class="flex-1">{{ $t('features.ai.query') }}</span>
                            <UiBadge variant="secondary">{{ $t('features.ai.smart') }}</UiBadge>
                        </div>
                        <div class="grid grid-cols-3 gap-3">
                            <div
                                v-for="result in searchResults"
                                :key="result.name"
                                class="flex flex-col gap-1.5"
                            >
                                <span
                                    :class="cn('h-20 rounded-2xl bg-linear-to-b', result.class)"
                                />
                                <TextBlock variant="muted" class="truncate text-xs">
                                    {{ result.name }}
                                </TextBlock>
                            </div>
                        </div>
                    </UiCardContent>
                </UiCard>

                <UiCard class="lg:col-span-3">
                    <UiCardHeader class="gap-2">
                        <div class="flex items-center gap-2 text-chart-1">
                            <ILock class="size-5" />
                            <TextBlock variant="small">
                                {{ $t('features.sharing.label') }}
                            </TextBlock>
                        </div>
                        <UiCardTitle class="text-xl font-semibold">
                            {{ $t('features.sharing.title') }}
                        </UiCardTitle>
                        <UiCardDescription class="text-base leading-relaxed">
                            {{ $t('features.sharing.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent class="mt-auto">
                        <div
                            class="divide-y overflow-hidden rounded-3xl border bg-muted/40 text-sm"
                        >
                            <div class="flex items-center justify-between px-4 py-3">
                                <span>{{ $t('features.sharing.password') }}</span>
                                <span class="font-mono text-muted-foreground">••••••••</span>
                            </div>
                            <div class="flex items-center justify-between px-4 py-3">
                                <span>{{ $t('features.sharing.expires') }}</span>
                                <span class="text-muted-foreground">
                                    {{ $t('features.sharing.expiresValue') }}
                                </span>
                            </div>
                            <div class="flex items-center justify-between px-4 py-3">
                                <span>{{ $t('features.sharing.maxViews') }}</span>
                                <span class="text-muted-foreground">10</span>
                            </div>
                            <div class="flex items-center justify-between px-4 py-3">
                                <span>{{ $t('features.sharing.discordEmbed') }}</span>
                                <UiSwitch
                                    :model-value="true"
                                    :aria-label="$t('features.sharing.discordEmbed')"
                                />
                            </div>
                        </div>
                    </UiCardContent>
                </UiCard>

                <UiCard class="lg:col-span-2">
                    <UiCardHeader class="gap-3">
                        <IDatabase class="size-5 text-chart-1" />
                        <UiCardTitle class="text-lg font-semibold">
                            {{ $t('features.storage.title') }}
                        </UiCardTitle>
                        <UiCardDescription class="text-base leading-relaxed">
                            {{ $t('features.storage.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent class="mt-auto flex flex-wrap gap-1.5">
                        <UiBadge
                            v-for="storage in storages"
                            :key="storage"
                            variant="outline"
                            class="font-mono"
                        >
                            {{ storage }}
                        </UiBadge>
                    </UiCardContent>
                </UiCard>

                <UiCard class="lg:col-span-2">
                    <UiCardHeader class="gap-3">
                        <IRefreshCw class="size-5 text-chart-1" />
                        <UiCardTitle class="text-lg font-semibold">
                            {{ $t('features.updates.title') }}
                        </UiCardTitle>
                        <UiCardDescription class="text-base leading-relaxed">
                            {{ $t('features.updates.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                    <UiCardContent class="mt-auto flex items-center gap-2">
                        <UiBadge variant="secondary" class="font-mono">{{ versions[0] }}</UiBadge>
                        <IArrowRight class="size-4 text-muted-foreground" />
                        <UiBadge class="font-mono">{{ versions[1] }}</UiBadge>
                    </UiCardContent>
                </UiCard>

                <UiCard class="lg:col-span-2">
                    <UiCardHeader class="gap-3">
                        <IUsers class="size-5 text-chart-1" />
                        <UiCardTitle class="text-lg font-semibold">
                            {{ $t('features.users.title') }}
                        </UiCardTitle>
                        <UiCardDescription class="text-base leading-relaxed">
                            {{ $t('features.users.description') }}
                        </UiCardDescription>
                    </UiCardHeader>
                </UiCard>
            </div>
        </div>
    </section>
</template>

<script setup lang="ts">
import {
    IFileText,
    IFolder,
    IMoveUpRight,
    IPencil,
    IScan,
    ISquare,
    IStickyNote,
    IType,
} from '#components';

const { t } = useI18n();

const tools = [IScan, ISquare, IMoveUpRight, IType, IPencil];

const copiedLink = 'files.example.com/u/k3Vx9.png';
const harborName = 'Fileship Harbor';
const versions = ['blue 6.4.0', 'green 6.5.0'];

const harborItems = computed(() => [
    { name: t('features.harbor.screenshots'), icon: IFolder, online: false },
    { name: t('features.harbor.projects'), icon: IFolder, online: false },
    { name: t('features.harbor.notes'), icon: IStickyNote, online: false },
    { name: 'invoice-2026.pdf', icon: IFileText, online: true },
]);

const storages = computed(() => [t('features.storage.local'), 'S3', 'SFTP', 'FTP']);

const searchResults = [
    { name: 'alpine-sunrise.jpg', class: 'from-chart-1 via-chart-3 to-chart-5' },
    { name: 'mountain-lake.jpg', class: 'from-chart-2 to-background' },
    { name: 'lavender-hills.jpg', class: 'from-chart-1 to-chart-4' },
];
</script>
