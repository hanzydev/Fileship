<template>
    <div class="overflow-hidden rounded-4xl border">
        <UiTable :class="columns.length > 2 && 'min-w-3xl'">
            <UiTableHeader>
                <UiTableRow class="hover:bg-transparent">
                    <UiTableHead class="w-1/3 pl-6" />
                    <UiTableHead
                        v-for="(app, index) in columns"
                        :key="app"
                        :class="
                            index === 0 ? 'bg-primary/15 font-semibold' : 'text-muted-foreground'
                        "
                    >
                        <NuxtLink
                            v-if="index > 0 && linked"
                            :to="`/alternatives/${app.toLowerCase()}`"
                            class="underline-offset-4 hover:text-foreground hover:underline"
                        >
                            {{ app }}
                        </NuxtLink>
                        <template v-else>{{ app }}</template>
                    </UiTableHead>
                </UiTableRow>
            </UiTableHeader>
            <UiTableBody>
                <UiTableRow v-for="row in visibleRows" :key="row.key" class="hover:bg-transparent">
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
</template>

<script setup lang="ts">
const { apps = COMPARE_APPS, linked = false } = defineProps<{
    apps?: string[];
    linked?: boolean;
}>();

const rows = useCompareRows();

const columns = computed(() => COMPARE_APPS.filter((app) => apps.includes(app)));

const visibleRows = computed(() =>
    rows.value.map((row) => ({
        ...row,
        cells: row.cells.filter((_, index) => apps.includes(COMPARE_APPS[index]!)),
    })),
);
</script>
