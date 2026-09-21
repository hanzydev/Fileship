<template>
    <Head>
        <Title>{{ error?.message }} | {{ error?.status }}</Title>
    </Head>

    <Centered class="text-left">
        <div class="space-y-4">
            <div class="space-y-2">
                <TextBlock variant="h1">
                    {{ error?.status }}
                </TextBlock>
                <TextBlock variant="h1">
                    {{ title }}
                </TextBlock>
            </div>

            <UiSeparator />

            <TextBlock variant="muted">
                {{
                    error?.status === 403
                        ? "Houston you... you aren't houston!"
                        : 'Houston, we have a problem.'
                }}
                {{ error?.message }}.
            </TextBlock>

            <UiButton
                size="lg"
                @click="canHistoryComeBack ? router.back() : router.push('/dashboard')"
            >
                {{ canHistoryComeBack ? 'Go back' : 'Go to dashboard' }}
                <IArrowRight :size="20" />
            </UiButton>
        </div>
    </Centered>
</template>

<script setup lang="ts">
const error = useError();
const router = useRouter();

const title = computed(
    () =>
        ({
            401: 'Unauthorized',
            403: 'Access denied',
            404: 'Lost in space',
            500: 'Internal server error',
        })[error.value!.status!] || 'Error',
);

const canHistoryComeBack = computed(() => window?.history?.state?.back);
</script>
