<template>
    <section class="pb-20 lg:pb-28">
        <div class="container mx-auto flex flex-col gap-10">
            <TextBlock v-reveal variant="h3" class="text-3xl">{{ $t('toolbox.title') }}</TextBlock>
            <div
                v-reveal
                class="grid grid-cols-1 gap-px overflow-hidden rounded-4xl border bg-border sm:grid-cols-2 lg:grid-cols-3"
            >
                <div v-for="item in items" :key="item.key" class="flex gap-4 bg-background p-6">
                    <component :is="item.icon" class="mt-0.5 size-5 shrink-0 text-chart-1" />
                    <div class="flex flex-col gap-1.5">
                        <TextBlock variant="small" class="text-base">
                            {{ $t(`toolbox.${item.key}`) }}
                        </TextBlock>
                        <TextBlock variant="muted" class="leading-relaxed">
                            {{ $t(`toolbox.${item.key}Text`) }}
                        </TextBlock>
                    </div>
                </div>
            </div>
        </div>
    </section>
</template>

<script setup lang="ts">
import {
    IArchive,
    ICopy,
    IInbox,
    ILanguages,
    ILink,
    INotebookPen,
    ISmartphone,
    ITrash2,
    IWebhook,
} from '#components';

const items = [
    { icon: INotebookPen, key: 'notes' },
    { icon: ILink, key: 'links' },
    { icon: IInbox, key: 'inboxes' },
    { icon: ITrash2, key: 'trash' },
    { icon: IArchive, key: 'backups' },
    { icon: IWebhook, key: 'webhooks' },
    { icon: ISmartphone, key: 'phone' },
    { icon: ICopy, key: 'duplicates' },
    { icon: ILanguages, key: 'languages' },
];
</script>
