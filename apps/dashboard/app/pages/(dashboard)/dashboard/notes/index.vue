<template>
    <UiCard class="flex-1 justify-center">
        <EmptyState
            :icon="IBookmark"
            :title="$t('notes.select')"
            :description="$t('notes.selectDescription')"
        >
            <UiButton as-child>
                <NuxtLink to="/dashboard/notes/new">
                    <ISquarePen />
                    {{ $t('notes.newNote') }}
                </NuxtLink>
            </UiButton>
        </EmptyState>
    </UiCard>
</template>

<script setup lang="ts">
import { IBookmark } from '#components';
</script>
