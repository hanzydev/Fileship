<template>
    <section id="getStarted" class="scroll-mt-20 pb-20 lg:pb-28">
        <div class="container mx-auto">
            <UiCard
                class="flex-col gap-10 p-6 sm:p-8 lg:flex-row lg:items-center lg:gap-12 lg:p-16"
            >
                <div class="flex flex-1 flex-col gap-5">
                    <SectionHeading>
                        <template #eyebrow>{{ $t('getStarted.eyebrow') }}</template>
                        <template #title>{{ $t('getStarted.title') }}</template>
                        <template #description>
                            <i18n-t keypath="getStarted.description" scope="global">
                                <template #address>
                                    <TextBlock variant="inlineCode">{{ address }}</TextBlock>
                                </template>
                            </i18n-t>
                        </template>
                    </SectionHeading>
                    <div class="flex flex-wrap gap-3">
                        <UiButton size="lg" as-child>
                            <NuxtLink to="/docs/installation">
                                {{ $t('getStarted.installation') }}
                            </NuxtLink>
                        </UiButton>
                        <UiButton size="lg" variant="outline" as-child>
                            <NuxtLink to="/docs/upgrading">
                                {{ $t('getStarted.upgrading') }}
                            </NuxtLink>
                        </UiButton>
                    </div>
                </div>

                <UiCodeBlock v-reveal class="min-w-0 flex-1">
                    <UiCodeBlockHeader>
                        <UiCodeBlockTitle>
                            <ITerminal />
                            {{ $t('getStarted.terminal') }}
                        </UiCodeBlockTitle>
                        <UiCodeBlockActions>
                            <CodeCopyButton :value="script" />
                        </UiCodeBlockActions>
                    </UiCodeBlockHeader>
                    <UiCodeBlockContent class="leading-8">
                        <UiCodeBlockLine v-for="command in commands" :key="command">
                            <span class="text-muted-foreground select-none">{{ prompt }}</span>
                            {{ command }}
                        </UiCodeBlockLine>
                        <UiCodeBlockLine class="text-muted-foreground">
                            <ICheck class="mr-2 inline size-4 text-chart-1" />
                            {{ $t('getStarted.ready', { url: `http://${address}` }) }}
                        </UiCodeBlockLine>
                    </UiCodeBlockContent>
                </UiCodeBlock>
            </UiCard>
        </div>
    </section>
</template>

<script setup lang="ts">
const address = 'localhost:3000';
const prompt = '$';

const commands = [
    'git clone https://github.com/hanzydev/Fileship.git',
    'cd Fileship',
    'echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env',
    'docker compose up -d',
];

const script = commands.join('\n');
</script>
