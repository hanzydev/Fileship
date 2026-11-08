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

                <div class="min-w-0 flex-1 overflow-hidden rounded-3xl border bg-background">
                    <div class="flex items-center justify-between border-b py-2 pr-2 pl-5">
                        <div class="flex items-center gap-2 text-muted-foreground">
                            <ITerminal class="size-4" />
                            <TextBlock variant="muted" class="font-mono text-xs">
                                {{ $t('getStarted.terminal') }}
                            </TextBlock>
                        </div>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="copied ? $t('getStarted.copied') : $t('getStarted.copy')"
                            @click="handleCopy"
                        >
                            <ICheck v-if="copied" />
                            <ICopy v-else />
                        </UiButton>
                    </div>
                    <pre
                        class="overflow-x-auto p-6 font-mono text-sm leading-8"
                    ><span v-for="command in commands" :key="command" class="block"><span class="text-muted-foreground select-none">{{ prompt }}</span>{{ command }}</span><span class="block text-muted-foreground"><ICheck class="mr-2 inline size-4 text-chart-1" />{{ $t('getStarted.ready', { url: `http://${address}` }) }}</span></pre>
                </div>
            </UiCard>
        </div>
    </section>
</template>

<script setup lang="ts">
const address = 'localhost:3000';
const prompt = '$ ';

const commands = [
    'git clone https://github.com/hanzydev/Fileship.git',
    'cd Fileship',
    'echo "SECRET_TOKEN=$(openssl rand -hex 32)" > .env',
    'docker compose up -d',
];

const { copy, copied } = useClipboard({ legacy: true });

const handleCopy = () => copy(commands.join('\n'));
</script>
