<template>
    <UiDropdownMenu>
        <UiDropdownMenuTrigger as-child>
            <UiButton variant="ghost" size="icon-sm" :aria-label="$t('common.more')">
                <IEllipsis />
            </UiButton>
        </UiDropdownMenuTrigger>
        <UiDropdownMenuContent align="end" class="w-56">
            <UiDropdownMenuItem as-child>
                <a :href="link.url" target="_blank" rel="noopener noreferrer">
                    <IExternalLink />
                    {{ $t('links.openTarget') }}
                </a>
            </UiDropdownMenuItem>
            <UiDropdownMenuItem @select="emit('edit')">
                <IPencil />
                {{ $t('links.edit') }}
            </UiDropdownMenuItem>
            <UiDropdownMenuSeparator />
            <UiDropdownMenuItem variant="destructive" @select="emit('delete')">
                <ITrash2 />
                {{ $t('links.delete') }}
            </UiDropdownMenuItem>
        </UiDropdownMenuContent>
    </UiDropdownMenu>
</template>

<script setup lang="ts">
import type { LinkData } from '~/types/links';

defineProps<{
    link: LinkData;
}>();

const emit = defineEmits<{
    edit: [];
    delete: [];
}>();
</script>
