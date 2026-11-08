<template>
    <section id="compare" class="scroll-mt-20 pb-20 lg:pb-28">
        <div class="container mx-auto flex flex-col gap-10">
            <SectionHeading>
                <template #eyebrow>{{ $t('compare.eyebrow') }}</template>
                <template #title>{{ $t('compare.title') }}</template>
                <template #description>
                    <i18n-t keypath="compare.description" scope="global">
                        <template #link>
                            <NuxtLink
                                to="/docs/migrating"
                                class="text-foreground underline underline-offset-4 hover:text-chart-1"
                            >
                                {{ $t('compare.migrate') }}
                            </NuxtLink>
                        </template>
                    </i18n-t>
                </template>
            </SectionHeading>

            <div class="overflow-hidden rounded-4xl border">
                <UiTable class="min-w-3xl">
                    <UiTableHeader>
                        <UiTableRow class="hover:bg-transparent">
                            <UiTableHead class="w-1/3 pl-6" />
                            <UiTableHead
                                v-for="(app, index) in apps"
                                :key="app"
                                :class="
                                    index === 0
                                        ? 'bg-primary/15 font-semibold'
                                        : 'text-muted-foreground'
                                "
                            >
                                {{ app }}
                            </UiTableHead>
                        </UiTableRow>
                    </UiTableHeader>
                    <UiTableBody>
                        <UiTableRow v-for="row in rows" :key="row.key" class="hover:bg-transparent">
                            <UiTableCell class="pl-6 text-muted-foreground">
                                {{ $t(`compare.rows.${row.key}`) }}
                            </UiTableCell>
                            <UiTableCell
                                v-for="(cell, index) in row.cells"
                                :key="index"
                                :class="index === 0 && 'bg-primary/15'"
                            >
                                <div class="flex items-center gap-2">
                                    <ICheck
                                        v-if="cell.kind === 'yes'"
                                        role="img"
                                        :aria-label="$t('compare.yes')"
                                        class="size-4 text-chart-1"
                                    />
                                    <IX
                                        v-else-if="cell.kind === 'no'"
                                        role="img"
                                        :aria-label="$t('compare.no')"
                                        class="size-4 text-muted-foreground"
                                    />
                                    <IMinus
                                        v-else-if="cell.kind === 'part'"
                                        role="img"
                                        :aria-label="$t('compare.partly')"
                                        class="size-4 text-chart-2"
                                    />
                                    <span v-if="cell.note" class="text-muted-foreground">
                                        {{ cell.note }}
                                    </span>
                                </div>
                            </UiTableCell>
                        </UiTableRow>
                    </UiTableBody>
                </UiTable>
            </div>
        </div>
    </section>
</template>

<script setup lang="ts">
const { t } = useI18n();

const yes = (note = '') => ({ kind: 'yes', note });
const no = (note = '') => ({ kind: 'no', note });
const part = (note = '') => ({ kind: 'part', note });
const text = (note = '') => ({ kind: 'text', note });

const apps = ['Fileship', 'Zipline', 'XBackBone', 'Chibisafe'];

const rows = computed(() => {
    const note = (key = '') => t(`compare.notes.${key}`);
    const local = note('local');

    return [
        { key: 'uploaders', cells: [yes(), yes(), yes(), yes()] },
        { key: 'desktop', cells: [yes('Windows'), no(), no(), no()] },
        { key: 'explorer', cells: [yes('Harbor'), no(), no(), no()] },
        { key: 'extension', cells: [yes(), no(), no(), yes()] },
        { key: 'ai', cells: [yes(note('onYourServer')), no(), no(), no()] },
        {
            key: 'folders',
            cells: [
                yes(note('nestedShared')),
                yes(note('nested')),
                no(note('tags')),
                part(note('flatAlbums')),
            ],
        },
        { key: 'sharing', cells: [yes(), yes(), no(), part(note('albumLinks'))] },
        {
            key: 'storage',
            cells: [
                text(`${local}, S3, SFTP, FTP`),
                text(`${local}, S3`),
                text(`${local}, S3, FTP, +2`),
                text(`${local}, S3`),
            ],
        },
        { key: 'security', cells: [yes(), yes(), no(), no()] },
        { key: 'sso', cells: [yes('OIDC'), yes('OAuth, OIDC'), yes('LDAP'), no()] },
        { key: 'webhooks', cells: [yes('Discord, Slack, JSON'), yes('Discord, HTTP'), no(), no()] },
        { key: 'backups', cells: [yes(), part(note('export')), part(note('export')), no()] },
        {
            key: 'updates',
            cells: [yes(note('zeroDowntime')), no(), part(note('withDowntime')), no()],
        },
        { key: 'languages', cells: [text('12'), text('12'), text(note('somePartial')), text('1')] },
        { key: 'license', cells: [text('MIT'), text('MIT'), text('AGPL-3.0'), text('MIT')] },
    ];
});
</script>
