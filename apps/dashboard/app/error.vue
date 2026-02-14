<template>
    <Head>
        <Title>{{ error?.message }} | {{ error?.status }}</Title>
    </Head>

    <Body hfull wfull bg-fs-background text-white antialiased>
        <UiCentered text-left>
            <div space-y-4>
                <div space-y-2>
                    <h1 select-none font-medium="!" md:text-5xl="!">
                        {{ error?.status }}
                    </h1>
                    <h1 md:text-5xl="!">
                        {{ title }}
                    </h1>
                </div>

                <div h1 rounded-full bg-fs-accent></div>
                <p text-fs-muted-1 font-medium>
                    {{
                        error?.status === 403
                            ? "Houston you... you aren't houston!"
                            : 'Houston, we have a problem.'
                    }}
                    {{ error?.message }}.
                </p>

                <UiButton
                    variant="accent"
                    wfit
                    flex-row-reverse
                    gap2
                    px6
                    rounded-2xl="!"
                    icon="solar:arrow-right-linear"
                    icon-size="20"
                    @click="canHistoryComeBack ? router.back() : router.push('/dashboard')"
                >
                    Go back
                </UiButton>
            </div>
        </UiCentered>
    </Body>
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
