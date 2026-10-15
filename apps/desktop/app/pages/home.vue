<template>
    <Head>
        <Title>Home</Title>
    </Head>

    <div class="flex flex-1 flex-col gap-4 p-4 pt-0">
        <PageHeader
            :title="`Welcome, ${account?.user.username}`"
            :description="`Connected to ${account?.siteName}.`"
        >
            <UiButton variant="outline" @click="handleOpenDashboard">
                <IExternalLink />
                Open dashboard
            </UiButton>
        </PageHeader>

        <EmptyState
            :icon="IImage"
            title="No captures yet"
            description="Your screenshots, recordings and uploads will show up here."
        />
    </div>
</template>

<script setup lang="ts">
import { IImage } from '#components';

const account = useAccount();

const handleOpenDashboard = () => desktop('app:openDashboard');

definePageMeta({
    layout: 'desktop',
});
</script>
