<template>
    <Head>
        <Title>{{ error.statusMessage }} | {{ error.statusCode }}</Title>
    </Head>

    <Body hfull wfull bg-fs5 text-white font-sans antialiased>
        <UiCentered text-left>
            <div space-y-4>
                <div space-y-2>
                    <h1 select-none font-medium="!" md:text-5xl="!">
                        {{ error.statusCode }}
                    </h1>
                    <h1 md:text-5xl="!">
                        {{ title }}
                    </h1>
                </div>

                <div h1 rounded-full bg-fs-accent></div>
                <p text-slate200 font-medium>
                    Houston, we have a problem. {{ error.message }}
                </p>

                <UiButton
                    variant="accent"
                    wfit
                    flex-row-reverse
                    gap2
                    px6
                    icon="heroicons-solid:arrow-right"
                    @click="
                        canHistoryComeBack
                            ? $router.back()
                            : $router.push('/dashboard')
                    "
                >
                    Go back
                </UiButton>
            </div>
        </UiCentered>
    </Body>
</template>

<script setup lang="ts">
import type { NuxtError } from '#app';

const { error } = defineProps<{
    error: NuxtError;
}>();

const title = computed(
    () =>
        ({
            401: 'Unauthorized',
            403: 'Access denied',
            404: 'Lost in space',
            500: 'Internal server error',
        })[error.statusCode] || 'Error',
);

const canHistoryComeBack = computed(() => window?.history?.state?.back);
</script>
