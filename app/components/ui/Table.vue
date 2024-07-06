<template>
    <div hfull overflow-auto rounded-md bg-fs3 p4>
        <table wfull>
            <thead>
                <tr>
                    <th
                        v-for="(col, index) in columns"
                        :key="index"
                        h10
                        whitespace-nowrap
                        bg-fs2
                        px4
                        text-left
                        text-slate300
                        first:rounded-l
                        last:rounded-r
                        font-semibold="!"
                        :style="{ width: col.width }"
                    >
                        {{ titleCase(col.key) }}
                    </th>
                </tr>
            </thead>
            <tbody>
                <tr v-for="row in rows" :key="row.key">
                    <td
                        v-for="(col, index) in columns"
                        :key="index"
                        whitespace-normal
                        px4
                        py2
                        text-slate200
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
            v-if="!rows.length && nothingHereMessage && nothingHereIcon"
            :message="nothingHereMessage"
            :icon="nothingHereIcon"
        />
    </div>
</template>

<script setup lang="ts">
import { titleCase } from 'scule';

defineProps<{
    columns: {
        key: string;
        width?: string;
        resolve?: (row: any) => string;
        render?: (row: any) => globalThis.VNode | string;
    }[];
    rows: Record<string, any>[];
    nothingHereMessage?: string;
    nothingHereIcon?: string;
}>();
</script>
