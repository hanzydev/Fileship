<template>
    <div>
        <Head>
            <Title>{{ $t('auth.oidc.title') }}</Title>
        </Head>
        <Centered>
            <UiCard class="w-full max-w-sm">
                <UiCardHeader>
                    <UiCardTitle>
                        {{
                            isVerified ? $t('auth.oidc.verifiedTitle') : $t('auth.oidc.failedTitle')
                        }}
                    </UiCardTitle>
                    <UiCardDescription>
                        {{
                            isVerified
                                ? $t('auth.oidc.verifiedDescription')
                                : $t(oidcErrorKey(status), { provider })
                        }}
                    </UiCardDescription>
                </UiCardHeader>
            </UiCard>
        </Centered>
    </div>
</template>

<script setup lang="ts">
const route = useRoute();
const provider = useOidcProvider();
const { post } = useBroadcastChannel<{ status: string }, { status: string }>({
    name: 'fileship-sso',
});

const status = typeof route.query.status === 'string' ? route.query.status : 'failed';
const isVerified = status === 'verified';

onMounted(() => {
    post({ status });
    if (isVerified) setTimeout(() => window.close(), 300);
});
</script>
