<template>
    <div>
        <Head>
            <Title>{{ $t('auth.verifyEmail.title') }}</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <UiCardHeader>
                    <UiCardTitle class="flex items-center gap-2">
                        <UiSpinner v-if="!result" />
                        <IMailCheck v-else-if="result.email" class="size-4" />
                        <ICircleAlert v-else class="size-4" />
                        {{
                            !result
                                ? $t('auth.verifyEmail.title')
                                : result.email
                                  ? $t('auth.verifyEmail.success')
                                  : $t('auth.verifyEmail.failed')
                        }}
                    </UiCardTitle>
                    <UiCardDescription>
                        {{
                            !result
                                ? $t('auth.verifyEmail.verifying')
                                : result.email
                                  ? $t('auth.verifyEmail.successDescription', {
                                        email: result.email,
                                    })
                                  : result.error
                        }}
                    </UiCardDescription>
                </UiCardHeader>
                <UiCardFooter v-if="result">
                    <UiButton class="w-full" as-child>
                        <NuxtLink :to="currentUser ? '/dashboard/settings/profile' : '/login'">
                            {{ $t('common.continue') }}
                        </NuxtLink>
                    </UiButton>
                </UiCardFooter>
            </UiCard>
        </Centered>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();
const currentUser = useAuthUser();

const result = ref<{ email?: string | null; error?: string }>();

onMounted(async () => {
    try {
        const { email } = await $fetch('/api/auth/verify-email', {
            method: 'POST',
            body: { token: route.query.token },
        });

        result.value = { email };
    } catch (error) {
        result.value = { error: getErrorMessage(error) };
    }
});
</script>
