<template>
    <section id="compare" class="scroll-mt-20 pb-20 lg:pb-28">
        <div class="container mx-auto flex flex-col gap-10">
            <SectionHeading>
                <template #eyebrow>{{ $t('compare.eyebrow') }}</template>
                <template #title>{{ $t('compare.title') }}</template>
                <template #description>
                    <i18n-t keypath="compare.description" scope="global">
                        <template #link>
                            <NuxtLink
                                to="/docs/migrating"
                                class="text-foreground underline underline-offset-4 hover:text-chart-1"
                            >
                                {{ $t('compare.migrate') }}
                            </NuxtLink>
                        </template>
                    </i18n-t>
                </template>
            </SectionHeading>

            <CompareTable linked />
        </div>
    </section>
</template>
