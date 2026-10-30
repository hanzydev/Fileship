<template>
    <div class="flex flex-col gap-4">
        <UiField orientation="horizontal">
            <UiSwitch :id="`${kind}-spoiler`" v-model="layout.spoiler" />
            <UiFieldLabel :for="`${kind}-spoiler`">
                {{ $t('sharing.discord.layoutSpoiler') }}
            </UiFieldLabel>
        </UiField>

        <UiCard v-for="(component, index) in layout.components" :key="keyOf(component)" size="sm">
            <UiCardHeader>
                <UiCardTitle>{{ $t(`sharing.discord.types.${component.type}`) }}</UiCardTitle>
                <UiCardAction class="flex gap-1">
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="$t('sharing.discord.moveUp')"
                        :disabled="index === 0"
                        @click="handleMove(index, -1)"
                    >
                        <IArrowUp />
                    </UiButton>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="$t('sharing.discord.moveDown')"
                        :disabled="index === layout.components.length - 1"
                        @click="handleMove(index, 1)"
                    >
                        <IArrowDown />
                    </UiButton>
                    <UiButton
                        variant="ghost"
                        size="icon-sm"
                        :aria-label="$t('sharing.discord.remove')"
                        :disabled="layout.components.length === 1"
                        @click="layout.components.splice(index, 1)"
                    >
                        <ITrash2 />
                    </UiButton>
                </UiCardAction>
            </UiCardHeader>

            <UiCardContent class="flex flex-col gap-3">
                <UiTextarea
                    v-if="component.type === 'text'"
                    v-model="component.content"
                    :aria-label="$t('sharing.discord.types.text')"
                    maxlength="2000"
                />

                <template v-else-if="component.type === 'section'">
                    <div
                        v-for="(_, textIndex) in component.texts"
                        :key="textIndex"
                        class="flex items-start gap-2"
                    >
                        <UiTextarea
                            v-model="component.texts[textIndex]"
                            :aria-label="$t('sharing.discord.sectionText', { n: textIndex + 1 })"
                            maxlength="2000"
                        />
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="$t('sharing.discord.removeText')"
                            :disabled="component.texts.length === 1"
                            @click="component.texts.splice(textIndex, 1)"
                        >
                            <IX />
                        </UiButton>
                    </div>
                    <UiButton
                        variant="outline"
                        size="sm"
                        class="self-start"
                        :disabled="component.texts.length === 3 || remaining < 1"
                        @click="component.texts.push('')"
                    >
                        <IPlus />
                        {{ $t('sharing.discord.addText') }}
                    </UiButton>

                    <UiField>
                        <UiFieldLabel>{{ $t('sharing.discord.accessory') }}</UiFieldLabel>
                        <UiSelect
                            :model-value="component.accessory.type"
                            @update:model-value="handleAccessory(component, $event as string)"
                        >
                            <UiSelectTrigger class="w-full sm:w-56">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem value="thumbnail">
                                    {{ $t('sharing.discord.thumbnail') }}
                                </UiSelectItem>
                                <UiSelectItem value="button">
                                    {{ $t('sharing.discord.button') }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>
                    <DiscordMediaFields
                        v-if="component.accessory.type === 'thumbnail'"
                        :id="`${kind}-${keyOf(component)}-accessory`"
                        :model-value="component.accessory"
                    />
                    <DiscordButtonFields
                        v-else
                        :id="`${kind}-${keyOf(component)}-accessory`"
                        :model-value="component.accessory"
                    />
                </template>

                <template v-else-if="component.type === 'gallery'">
                    <div
                        v-for="(item, itemIndex) in component.items"
                        :key="itemIndex"
                        class="flex items-start gap-2"
                    >
                        <DiscordMediaFields
                            :id="`${kind}-${keyOf(component)}-${itemIndex}`"
                            :model-value="item"
                            class="flex-1"
                        />
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="$t('sharing.discord.removeItem')"
                            :disabled="component.items.length === 1"
                            @click="component.items.splice(itemIndex, 1)"
                        >
                            <IX />
                        </UiButton>
                    </div>
                    <UiButton
                        variant="outline"
                        size="sm"
                        class="self-start"
                        :disabled="component.items.length === 10"
                        @click="component.items.push(newMedia())"
                    >
                        <IPlus />
                        {{ $t('sharing.discord.addItem') }}
                    </UiButton>
                </template>

                <div v-else-if="component.type === 'separator'" class="flex flex-wrap gap-6">
                    <UiField orientation="horizontal" class="w-auto">
                        <UiSwitch
                            :id="`${kind}-${keyOf(component)}-divider`"
                            v-model="component.divider"
                        />
                        <UiFieldLabel :for="`${kind}-${keyOf(component)}-divider`">
                            {{ $t('sharing.discord.divider') }}
                        </UiFieldLabel>
                    </UiField>
                    <UiField orientation="horizontal" class="w-auto">
                        <UiSwitch
                            :id="`${kind}-${keyOf(component)}-large`"
                            v-model="component.large"
                        />
                        <UiFieldLabel :for="`${kind}-${keyOf(component)}-large`">
                            {{ $t('sharing.discord.largeSpacing') }}
                        </UiFieldLabel>
                    </UiField>
                </div>

                <template v-else>
                    <div
                        v-for="(button, buttonIndex) in component.buttons"
                        :key="buttonIndex"
                        class="flex items-start gap-2"
                    >
                        <DiscordButtonFields
                            :id="`${kind}-${keyOf(component)}-${buttonIndex}`"
                            :model-value="button"
                            class="flex-1"
                        />
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="$t('sharing.discord.removeButton')"
                            :disabled="component.buttons.length === 1"
                            @click="component.buttons.splice(buttonIndex, 1)"
                        >
                            <IX />
                        </UiButton>
                    </div>
                    <UiButton
                        variant="outline"
                        size="sm"
                        class="self-start"
                        :disabled="component.buttons.length === 5 || remaining < 1"
                        @click="component.buttons.push(newButton())"
                    >
                        <IPlus />
                        {{ $t('sharing.discord.addButton') }}
                    </UiButton>
                </template>
            </UiCardContent>
        </UiCard>

        <div class="flex flex-wrap items-center gap-2">
            <UiSelect v-model="newType">
                <UiSelectTrigger class="w-44" :aria-label="$t('sharing.discord.componentType')">
                    <UiSelectValue />
                </UiSelectTrigger>
                <UiSelectContent>
                    <UiSelectItem v-for="type in TYPES" :key="type" :value="type">
                        {{ $t(`sharing.discord.types.${type}`) }}
                    </UiSelectItem>
                </UiSelectContent>
            </UiSelect>
            <UiButton variant="outline" :disabled="remaining < COST[newType]" @click="handleAdd">
                <IPlus />
                {{ $t('sharing.discord.add') }}
            </UiButton>
            <span class="text-sm text-muted-foreground">
                {{
                    $t('sharing.discord.componentCount', {
                        count: discordComponentCount(layout.components),
                        limit: DISCORD_COMPONENT_LIMIT,
                    })
                }}
            </span>
            <UiButton variant="ghost" class="ms-auto" @click="emit('reset')">
                <IRotateCcw />
                {{ $t('sharing.discord.reset') }}
            </UiButton>
        </div>

        <TextBlock variant="muted">
            {{ $t('sharing.discord.placeholders', { list: placeholders }) }}
        </TextBlock>
    </div>
</template>

<script setup lang="ts">
type ComponentType = DiscordChild['type'];

const TYPES: ComponentType[] = ['text', 'section', 'gallery', 'separator', 'buttons'];
const COST: Record<ComponentType, number> = {
    text: 1,
    section: 3,
    gallery: 1,
    separator: 1,
    buttons: 2,
};

const { kind } = defineProps<{ kind: DiscordEmbedKind }>();
const emit = defineEmits<{ reset: [] }>();

const layout = defineModel<DiscordLayout>({ required: true });

const newType = ref<ComponentType>('text');

const placeholders = DISCORD_PLACEHOLDERS[kind].map((name) => `{${name}}`).join(' ');

const remaining = computed(
    () => DISCORD_COMPONENT_LIMIT - discordComponentCount(layout.value.components),
);

const keys = new WeakMap<object, number>();
let nextKey = 0;

const keyOf = (component: object) => {
    if (!keys.has(component)) keys.set(component, nextKey++);
    return keys.get(component)!;
};

const newMedia = (): DiscordMedia => ({
    url: kind === 'file' ? '{fileUrl}' : '',
    description: '',
    spoiler: false,
});

const newButton = (): DiscordButton => ({ label: '', url: '{url}', emoji: '' });

const newComponent = (type: ComponentType): DiscordChild => {
    switch (type) {
        case 'text':
            return { type, content: '' };
        case 'section':
            return {
                type,
                texts: [''],
                accessory:
                    kind === 'file'
                        ? {
                              type: 'thumbnail',
                              url: '{thumbnailUrl}',
                              description: '',
                              spoiler: false,
                          }
                        : { type: 'button', ...newButton() },
            };
        case 'gallery':
            return { type, items: [newMedia()] };
        case 'separator':
            return { type, divider: true, large: false };
        case 'buttons':
            return { type, buttons: [newButton()] };
    }
};

const handleAdd = () => {
    layout.value.components.push(newComponent(newType.value));
};

const handleMove = (index: number, offset: number) => {
    const [component] = layout.value.components.splice(index, 1);
    layout.value.components.splice(index + offset, 0, component!);
};

const handleAccessory = (component: Extract<DiscordChild, { type: 'section' }>, type: string) => {
    component.accessory =
        type === 'thumbnail'
            ? {
                  type,
                  url: kind === 'file' ? '{thumbnailUrl}' : '',
                  description: '',
                  spoiler: false,
              }
            : { type: 'button', ...newButton() };
};
</script>
