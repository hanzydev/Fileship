<template>
    <div hfull overflow-auto rounded-xl bg-fs-overlay-2 p4>
        <table wfull>
            <thead>
                <tr>
                    <th
                        v-for="(col, index) in columns"
                        :key="index"
                        h10
                        whitespace-nowrap
                        bg-fs-overlay-3
                        px4
                        text-left
                        text-fs-muted-2
                        first:rounded-l-lg
                        last:rounded-r-lg
                        font-semibold="!"
                        :style="{ width: col.width }"
                    >
                        {{ titleCase(col.key) }}
                    </th>
                </tr>
            </thead>
            <tbody v-if="rows.length && !loading">
                <tr v-for="row in rows" :key="row.key">
                    <td
                        v-for="(col, index) in columns"
                        :key="index"
                        whitespace-normal
                        px4
                        py2
                        text-fs-muted-1
                        font-normal="!"
                    >
                        <component :is="col.render(row)" v-if="col.render" />
                        <template v-else>
                            {{ col?.resolve ? col.resolve(row) : row[col.key] }}
                        </template>
                    </td>
                </tr>
            </tbody>
        </table>
        <NothingHere
            v-if="!rows?.length && nothingHereMessage && nothingHereIcon && !loading"
            ring-none
            :message="nothingHereMessage"
            :icon="nothingHereIcon"
        />
        <Loading v-if="loading" ring-none />
    </div>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

const { rows = [] } = defineProps<{
    columns: {
        key: string;
        width?: string;
        resolve?: (row: any) => string;
        render?: (row: any) => globalThis.VNode | string;
    }[];
    rows?: Record<string, any>[];
    nothingHereMessage?: string;
    nothingHereIcon?: string;
    loading?: boolean;
}>();
</script>
