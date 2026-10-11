<template>
    <UiCard class="flex-1 justify-center">
        <EmptyState
            :icon="INotebookPen"
            title="Select a note"
            description="Pick a note from the list or start a new one."
        >
            <UiButton as-child>
                <NuxtLink to="/dashboard/notes/new">
                    <ISquarePen />
                    New note
                </NuxtLink>
            </UiButton>
        </EmptyState>
    </UiCard>
</template>

<script setup lang="ts">
import { INotebookPen } from '#components';
</script>
