<template>
    <Head>
        <Title>{{ title }} · {{ error?.status }}</Title>
    </Head>

    <Centered>
        <EmptyState
            :icon="error?.status === 404 ? ISearchX : ICircleAlert"
            :title="`${error?.status} · ${title}`"
            :description="error?.message"
        >
            <UiButton @click="canHistoryComeBack ? router.back() : router.push('/overview')">
                {{ canHistoryComeBack ? 'Go back' : 'Go to overview' }}
                <IArrowRight />
            </UiButton>
        </EmptyState>
    </Centered>
</template>

<script setup lang="ts">
import { ICircleAlert, ISearchX } from '#components';

const error = useError();
const router = useRouter();

const title = computed(
    () =>
        ({
            401: 'Unauthorized',
            403: 'Access denied',
            404: 'Page not found',
            500: 'Something went wrong',
        })[error.value!.status!] || 'Error',
);

const canHistoryComeBack = computed(() => import.meta.client && !!window.history.state?.back);
</script>
