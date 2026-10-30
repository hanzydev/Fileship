<template>
    <div class="flex flex-col gap-4">
        <div
            class="relative flex flex-col gap-3 overflow-hidden rounded-xl border bg-card p-3 ps-5 before:absolute before:inset-y-0 before:start-0 before:w-1 before:bg-(--accent)"
            :style="{ '--accent': color }"
        >
            <div class="flex flex-wrap items-center justify-between gap-2">
                <UiField orientation="horizontal" class="w-auto">
                    <UiSwitch :id="`${kind}-spoiler`" v-model="layout.spoiler" />
                    <UiFieldLabel :for="`${kind}-spoiler`">
                        {{ $t('sharing.discord.layoutSpoiler') }}
                    </UiFieldLabel>
                </UiField>
                <span class="text-sm text-muted-foreground">
                    {{
                        $t('sharing.discord.componentCount', {
                            count: discordComponentCount(layout.components),
                            limit: DISCORD_COMPONENT_LIMIT,
                        })
                    }}
                </span>
            </div>

            <UiCollapsible
                v-for="(component, index) in layout.components"
                :id="`${kind}-child-${keyOf(component)}`"
                :key="keyOf(component)"
                :open="openKeys.has(keyOf(component))"
                class="rounded-lg border bg-background"
                @update:open="handleOpen(component, $event)"
            >
                <div class="flex items-center gap-1 p-1 ps-2">
                    <UiCollapsibleTrigger
                        class="flex min-w-0 flex-1 items-center gap-2 rounded-md py-1 text-start text-sm outline-none focus-visible:ring-2 focus-visible:ring-ring"
                    >
                        <IChevronRight
                            :class="
                                cn(
                                    'size-4 shrink-0 text-muted-foreground transition-transform',
                                    openKeys.has(keyOf(component)) && 'rotate-90',
                                )
                            "
                        />
                        <component
                            :is="ICONS[component.type]"
                            class="size-4 shrink-0 text-muted-foreground"
                        />
                        <span class="shrink-0 font-medium">{{ ordinalLabel(index) }}</span>
                        <UiBadge
                            v-if="component.type === 'section'"
                            variant="secondary"
                            class="shrink-0"
                        >
                            {{ $t('sharing.discord.withAccessory') }}
                        </UiBadge>
                        <span class="truncate text-muted-foreground">{{ excerpt(component) }}</span>
                        <ICircleAlert
                            v-if="discordChildProblems(component).length"
                            class="size-4 shrink-0 text-destructive"
                        />
                    </UiCollapsibleTrigger>
                    <DiscordActionBar
                        :index="index"
                        :length="layout.components.length"
                        :can-copy="remaining >= costOf(component)"
                        :can-remove="layout.components.length > 1"
                        @move="handleMove(layout.components, index, $event)"
                        @copy="handleCopyComponent(index)"
                        @remove="layout.components.splice(index, 1)"
                    />
                </div>

                <UiCollapsibleContent>
                    <div class="flex flex-col gap-3 border-t p-3">
                        <UiAlert
                            v-if="discordChildProblems(component).length"
                            variant="destructive"
                        >
                            <ICircleAlert />
                            <UiAlertDescription>
                                <p
                                    v-for="problem in discordChildProblems(component)"
                                    :key="problem"
                                >
                                    {{ $t(`sharing.discord.problems.${problem}`) }}
                                </p>
                            </UiAlertDescription>
                        </UiAlert>

                        <template v-if="component.type === 'text'">
                            <UiTextarea
                                v-model="component.content"
                                :aria-label="ordinalLabel(index)"
                                maxlength="2000"
                            />
                            <UiDropdownMenu>
                                <UiDropdownMenuTrigger as-child>
                                    <UiButton
                                        variant="outline"
                                        size="sm"
                                        class="self-start"
                                        :disabled="remaining < 2"
                                    >
                                        <IPlus />
                                        {{ $t('sharing.discord.addAccessory') }}
                                    </UiButton>
                                </UiDropdownMenuTrigger>
                                <UiDropdownMenuContent align="start">
                                    <UiDropdownMenuItem
                                        @select="handleAddAccessory(index, 'button')"
                                    >
                                        <IExternalLink />
                                        {{ $t('sharing.discord.linkButton') }}
                                    </UiDropdownMenuItem>
                                    <UiDropdownMenuItem
                                        @select="handleAddAccessory(index, 'thumbnail')"
                                    >
                                        <IImage />
                                        {{ $t('sharing.discord.thumbnail') }}
                                    </UiDropdownMenuItem>
                                </UiDropdownMenuContent>
                            </UiDropdownMenu>
                        </template>

                        <template v-else-if="component.type === 'section'">
                            <div
                                v-for="(_, textIndex) in component.texts"
                                :key="textIndex"
                                class="flex items-start gap-1"
                            >
                                <UiTextarea
                                    v-model="component.texts[textIndex]"
                                    :aria-label="ordinalLabel(index)"
                                    maxlength="2000"
                                />
                                <div class="flex flex-col">
                                    <UiButton
                                        variant="ghost"
                                        size="icon-sm"
                                        :aria-label="$t('sharing.discord.remove')"
                                        :disabled="component.texts.length === 1"
                                        @click="component.texts.splice(textIndex, 1)"
                                    >
                                        <ITrash2 />
                                    </UiButton>
                                    <UiButton
                                        variant="ghost"
                                        size="icon-sm"
                                        :aria-label="$t('sharing.discord.addTextBelow')"
                                        :disabled="component.texts.length === 3 || remaining < 1"
                                        @click="component.texts.splice(textIndex + 1, 0, '')"
                                    >
                                        <IPlus />
                                    </UiButton>
                                </div>
                            </div>

                            <UiSeparator />

                            <div class="flex items-center gap-2">
                                <span class="text-sm font-medium">
                                    {{
                                        $t('sharing.discord.accessoryOf', {
                                            type:
                                                component.accessory.type === 'thumbnail'
                                                    ? $t('sharing.discord.thumbnail')
                                                    : $t('sharing.discord.linkButton'),
                                        })
                                    }}
                                </span>
                                <UiButton
                                    variant="ghost"
                                    size="icon-sm"
                                    class="ms-auto"
                                    :aria-label="$t('sharing.discord.removeAccessory')"
                                    @click="handleRemoveAccessory(index)"
                                >
                                    <ITrash2 />
                                </UiButton>
                            </div>

                            <template v-if="component.accessory.type === 'thumbnail'">
                                <UiField>
                                    <UiFieldLabel :for="fieldId(component, 'url')">
                                        {{ $t('sharing.discord.mediaUrl') }}
                                    </UiFieldLabel>
                                    <UiInput
                                        :id="fieldId(component, 'url')"
                                        v-model="component.accessory.url"
                                        maxlength="2048"
                                    />
                                </UiField>
                                <div
                                    v-if="component.accessory.url.trim()"
                                    class="grid gap-3 sm:grid-cols-[1fr_auto] sm:items-end"
                                >
                                    <UiField>
                                        <UiFieldLabel :for="fieldId(component, 'description')">
                                            {{ $t('sharing.discord.altText') }}
                                        </UiFieldLabel>
                                        <UiInput
                                            :id="fieldId(component, 'description')"
                                            v-model="component.accessory.description"
                                            maxlength="1024"
                                        />
                                    </UiField>
                                    <UiField orientation="horizontal" class="w-auto sm:h-9">
                                        <UiSwitch
                                            :id="fieldId(component, 'spoiler')"
                                            v-model="component.accessory.spoiler"
                                        />
                                        <UiFieldLabel :for="fieldId(component, 'spoiler')">
                                            {{ $t('sharing.discord.spoiler') }}
                                        </UiFieldLabel>
                                    </UiField>
                                </div>
                            </template>
                            <div v-else class="grid gap-3 sm:grid-cols-[1fr_5rem_1fr]">
                                <UiField>
                                    <UiFieldLabel :for="fieldId(component, 'label')">
                                        {{ $t('sharing.discord.buttonLabel') }}
                                    </UiFieldLabel>
                                    <UiInput
                                        :id="fieldId(component, 'label')"
                                        v-model="component.accessory.label"
                                        maxlength="80"
                                    />
                                </UiField>
                                <UiField>
                                    <UiFieldLabel :for="fieldId(component, 'emoji')">
                                        {{ $t('sharing.discord.emoji') }}
                                    </UiFieldLabel>
                                    <UiInput
                                        :id="fieldId(component, 'emoji')"
                                        v-model="component.accessory.emoji"
                                        maxlength="32"
                                    />
                                </UiField>
                                <UiField>
                                    <UiFieldLabel :for="fieldId(component, 'link')">
                                        {{ $t('sharing.discord.buttonUrl') }}
                                    </UiFieldLabel>
                                    <UiInput
                                        :id="fieldId(component, 'link')"
                                        v-model="component.accessory.url"
                                        maxlength="2048"
                                    />
                                </UiField>
                            </div>
                        </template>

                        <template v-else-if="component.type === 'gallery'">
                            <template
                                v-for="(item, itemIndex) in component.items"
                                :key="keyOf(item)"
                            >
                                <UiSeparator v-if="itemIndex" />
                                <div class="flex flex-col gap-2">
                                    <div class="flex min-w-0 items-center gap-2">
                                        <span class="shrink-0 text-sm font-medium">
                                            {{
                                                $t('sharing.discord.ordinal.media', {
                                                    n: itemIndex + 1,
                                                })
                                            }}
                                        </span>
                                        <span class="truncate text-sm text-muted-foreground">
                                            {{ item.description || item.url }}
                                        </span>
                                        <DiscordActionBar
                                            class="ms-auto"
                                            :index="itemIndex"
                                            :length="component.items.length"
                                            :can-copy="component.items.length < 10"
                                            :can-remove="component.items.length > 1"
                                            @move="handleMove(component.items, itemIndex, $event)"
                                            @copy="handleCopy(component.items, itemIndex)"
                                            @remove="component.items.splice(itemIndex, 1)"
                                        />
                                    </div>
                                    <UiInput
                                        v-model="item.url"
                                        :aria-label="$t('sharing.discord.mediaUrl')"
                                        maxlength="2048"
                                    />
                                    <div
                                        v-if="item.url.trim()"
                                        class="grid gap-3 sm:grid-cols-[1fr_auto] sm:items-end"
                                    >
                                        <UiField>
                                            <UiFieldLabel :for="fieldId(item, 'description')">
                                                {{ $t('sharing.discord.altText') }}
                                            </UiFieldLabel>
                                            <UiInput
                                                :id="fieldId(item, 'description')"
                                                v-model="item.description"
                                                maxlength="1024"
                                            />
                                        </UiField>
                                        <UiField orientation="horizontal" class="w-auto sm:h-9">
                                            <UiSwitch
                                                :id="fieldId(item, 'spoiler')"
                                                v-model="item.spoiler"
                                            />
                                            <UiFieldLabel :for="fieldId(item, 'spoiler')">
                                                {{ $t('sharing.discord.spoiler') }}
                                            </UiFieldLabel>
                                        </UiField>
                                    </div>
                                </div>
                            </template>
                            <UiButton
                                variant="outline"
                                size="sm"
                                class="self-start"
                                :disabled="component.items.length === 10"
                                @click="component.items.push(newMedia())"
                            >
                                <IPlus />
                                {{ $t('sharing.discord.addMedia') }}
                            </UiButton>
                        </template>

                        <div
                            v-else-if="component.type === 'separator'"
                            class="flex flex-wrap gap-6"
                        >
                            <UiField orientation="horizontal" class="w-auto">
                                <UiSwitch
                                    :id="fieldId(component, 'divider')"
                                    v-model="component.divider"
                                />
                                <UiFieldLabel :for="fieldId(component, 'divider')">
                                    {{ $t('sharing.discord.divider') }}
                                </UiFieldLabel>
                            </UiField>
                            <UiField orientation="horizontal" class="w-auto">
                                <UiSwitch
                                    :id="fieldId(component, 'large')"
                                    v-model="component.large"
                                />
                                <UiFieldLabel :for="fieldId(component, 'large')">
                                    {{ $t('sharing.discord.largeSpacing') }}
                                </UiFieldLabel>
                            </UiField>
                        </div>

                        <template v-else>
                            <template
                                v-for="(button, buttonIndex) in component.buttons"
                                :key="keyOf(button)"
                            >
                                <UiSeparator v-if="buttonIndex" />
                                <div class="flex flex-col gap-2">
                                    <div class="flex min-w-0 items-center gap-2">
                                        <span class="shrink-0 text-sm font-medium">
                                            {{
                                                $t('sharing.discord.ordinal.button', {
                                                    n: buttonIndex + 1,
                                                })
                                            }}
                                        </span>
                                        <span class="truncate text-sm text-muted-foreground">
                                            {{ buttonText(button) }}
                                        </span>
                                        <DiscordActionBar
                                            class="ms-auto"
                                            :index="buttonIndex"
                                            :length="component.buttons.length"
                                            :can-copy="
                                                component.buttons.length < 5 && remaining >= 1
                                            "
                                            :can-remove="component.buttons.length > 1"
                                            @move="
                                                handleMove(component.buttons, buttonIndex, $event)
                                            "
                                            @copy="handleCopy(component.buttons, buttonIndex)"
                                            @remove="component.buttons.splice(buttonIndex, 1)"
                                        />
                                    </div>
                                    <div class="grid gap-3 sm:grid-cols-[1fr_5rem_1fr]">
                                        <UiField>
                                            <UiFieldLabel :for="fieldId(button, 'label')">
                                                {{ $t('sharing.discord.buttonLabel') }}
                                            </UiFieldLabel>
                                            <UiInput
                                                :id="fieldId(button, 'label')"
                                                v-model="button.label"
                                                maxlength="80"
                                            />
                                        </UiField>
                                        <UiField>
                                            <UiFieldLabel :for="fieldId(button, 'emoji')">
                                                {{ $t('sharing.discord.emoji') }}
                                            </UiFieldLabel>
                                            <UiInput
                                                :id="fieldId(button, 'emoji')"
                                                v-model="button.emoji"
                                                maxlength="32"
                                            />
                                        </UiField>
                                        <UiField>
                                            <UiFieldLabel :for="fieldId(button, 'link')">
                                                {{ $t('sharing.discord.buttonUrl') }}
                                            </UiFieldLabel>
                                            <UiInput
                                                :id="fieldId(button, 'link')"
                                                v-model="button.url"
                                                maxlength="2048"
                                            />
                                        </UiField>
                                    </div>
                                </div>
                            </template>
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
                    </div>
                </UiCollapsibleContent>
            </UiCollapsible>

            <div class="flex flex-wrap items-center gap-2">
                <UiDropdownMenu>
                    <UiDropdownMenuTrigger as-child>
                        <UiButton variant="outline" :disabled="remaining < 1">
                            <IPlus />
                            {{ $t('sharing.discord.addComponent') }}
                        </UiButton>
                    </UiDropdownMenuTrigger>
                    <UiDropdownMenuContent align="start">
                        <UiDropdownMenuItem
                            v-for="type in ADD_TYPES"
                            :key="type"
                            :disabled="remaining < COST[type]"
                            @select="handleAdd(type)"
                        >
                            <component :is="ICONS[type]" />
                            {{ $t(`sharing.discord.types.${type}`) }}
                        </UiDropdownMenuItem>
                    </UiDropdownMenuContent>
                </UiDropdownMenu>
                <UiButton variant="ghost" class="ms-auto" @click="emit('reset')">
                    <IRotateCcw />
                    {{ $t('sharing.discord.reset') }}
                </UiButton>
            </div>
        </div>

        <TextBlock variant="muted">
            {{ $t('sharing.discord.placeholders', { list: placeholders }) }}
        </TextBlock>
    </div>
</template>

<script setup lang="ts">
import { IImages, IMinus, IMousePointerClick, IType } from '#components';

type ComponentType = DiscordChild['type'];
type AddType = Exclude<ComponentType, 'section'>;

const ICONS = {
    text: IType,
    section: IType,
    gallery: IImages,
    separator: IMinus,
    buttons: IMousePointerClick,
};
const ADD_TYPES: AddType[] = ['text', 'gallery', 'separator', 'buttons'];
const COST: Record<AddType, number> = { text: 1, gallery: 1, separator: 1, buttons: 2 };

const { kind, color } = defineProps<{ kind: DiscordEmbedKind; color: string }>();
const emit = defineEmits<{ reset: [] }>();

const layout = defineModel<DiscordLayout>({ required: true });

const { t } = useI18n();

const placeholders = DISCORD_PLACEHOLDERS[kind].map((name) => `{${name}}`).join(' ');

const remaining = computed(
    () => DISCORD_COMPONENT_LIMIT - discordComponentCount(layout.value.components),
);

const keys = new WeakMap<object, number>();
let nextKey = 0;

const keyOf = (value: object) => {
    const raw = toRaw(value);
    if (!keys.has(raw)) keys.set(raw, nextKey++);
    return keys.get(raw)!;
};

const fieldId = (value: object, field: string) => `${kind}-${keyOf(value)}-${field}`;

const openKeys = ref(new Set<number>());

const handleOpen = (component: DiscordChild, open: boolean) => {
    const key = keyOf(component);
    if (open) openKeys.value.add(key);
    else openKeys.value.delete(key);
};

const reveal = async (component: DiscordChild) => {
    handleOpen(component, true);
    await nextTick();
    document
        .getElementById(`${kind}-child-${keyOf(component)}`)
        ?.scrollIntoView({ block: 'nearest', behavior: 'smooth' });
};

const costOf = (component: DiscordChild) => discordComponentCount([component]) - 1;

const familyOf = (component: DiscordChild) =>
    component.type === 'section' ? 'text' : component.type;

const ordinalLabel = (index: number) => {
    const family = familyOf(layout.value.components[index]!);
    const n = layout.value.components
        .slice(0, index + 1)
        .filter((component) => familyOf(component) === family).length;

    return t(`sharing.discord.ordinal.${family}`, { n });
};

const firstLine = (text: string) =>
    text
        .split('\n')
        .find((line) => line.trim())
        ?.trim() ?? '';

const buttonText = (button: DiscordButton) =>
    [button.emoji, button.label].filter((part) => part.trim()).join(' ') || button.url;

const excerpt = (component: DiscordChild) => {
    switch (component.type) {
        case 'text':
            return firstLine(component.content);
        case 'section':
            return firstLine(component.texts[0]!);
        case 'gallery':
            return component.items
                .map((item) => item.description || item.url)
                .filter(Boolean)
                .join(', ');
        case 'separator':
            return [
                t(component.divider ? 'sharing.discord.excerptLine' : 'sharing.discord.excerptGap'),
                ...(component.large ? [t('sharing.discord.excerptLarge')] : []),
            ].join(', ');
        case 'buttons':
            return component.buttons.map(buttonText).join(', ');
    }
};

const newMedia = (): DiscordMedia => ({
    url: kind === 'file' ? '{fileUrl}' : '',
    description: '',
    spoiler: false,
});

const newButton = (): DiscordButton => ({ label: '', url: '{url}', emoji: '' });

const newComponent = (type: AddType): DiscordChild => {
    switch (type) {
        case 'text':
            return { type, content: '' };
        case 'gallery':
            return { type, items: [newMedia()] };
        case 'separator':
            return { type, divider: true, large: false };
        case 'buttons':
            return { type, buttons: [newButton()] };
    }
};

const handleAdd = (type: AddType) => {
    layout.value.components.push(newComponent(type));
    reveal(layout.value.components.at(-1)!);
};

const handleMove = <Item,>(list: Item[], index: number, offset: number) => {
    const [item] = list.splice(index, 1);
    list.splice(index + offset, 0, item!);
};

const handleCopy = <Item,>(list: Item[], index: number) => {
    list.splice(index + 1, 0, structuredClone(toRaw(list[index]!)));
};

const handleCopyComponent = (index: number) => {
    handleCopy(layout.value.components, index);
    reveal(layout.value.components[index + 1]!);
};

const handleAddAccessory = (index: number, type: 'thumbnail' | 'button') => {
    const text = layout.value.components[index] as Extract<DiscordChild, { type: 'text' }>;

    layout.value.components.splice(index, 1, {
        type: 'section',
        texts: [text.content],
        accessory:
            type === 'thumbnail'
                ? {
                      type,
                      url: kind === 'file' ? '{thumbnailUrl}' : '',
                      description: '',
                      spoiler: false,
                  }
                : { type, ...newButton() },
    });

    reveal(layout.value.components[index]!);
};

const handleRemoveAccessory = (index: number) => {
    const section = layout.value.components[index] as Extract<DiscordChild, { type: 'section' }>;

    layout.value.components.splice(index, 1, {
        type: 'text',
        content: section.texts.filter((text) => text.trim()).join('\n'),
    });

    reveal(layout.value.components[index]!);
};
</script>
