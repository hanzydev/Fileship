<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('hotkeys.title') }}</UiCardTitle>
            <UiCardDescription>{{ $t('hotkeys.description') }}</UiCardDescription>
            <UiCardAction>
                <UiSwitch
                    :model-value="settings?.hotkeysEnabled"
                    :disabled="!settings"
                    :aria-label="$t('hotkeys.enable')"
                    @update:model-value="handleToggle"
                />
            </UiCardAction>
        </UiCardHeader>
        <UiCardContent>
            <UiItemGroup :class="cn('gap-2', !settings?.hotkeysEnabled && 'opacity-50')">
                <UiItem v-for="action in HOTKEY_ACTIONS" :key="action" variant="muted" size="sm">
                    <UiItemContent class="min-w-0">
                        <UiItemTitle>{{ $t(`hotkeys.actions.${action}`) }}</UiItemTitle>
                        <UiItemDescription v-if="duplicates.has(action)" class="text-destructive">
                            {{ $t('hotkeys.duplicate') }}
                        </UiItemDescription>
                        <UiItemDescription
                            v-else-if="status[action] === 'conflict'"
                            class="text-destructive"
                        >
                            {{ $t('hotkeys.conflict') }}
                        </UiItemDescription>
                        <UiItemDescription
                            v-else-if="status[action] === 'invalid'"
                            class="text-destructive"
                        >
                            {{ $t('hotkeys.invalid') }}
                        </UiItemDescription>
                    </UiItemContent>
                    <UiItemActions>
                        <UiButton
                            :variant="recording === action ? 'default' : 'outline'"
                            size="sm"
                            class="min-w-32"
                            :disabled="!settings?.hotkeysEnabled"
                            @click="handleRecord(action)"
                        >
                            <template v-if="recording === action">
                                {{ $t('hotkeys.recording') }}
                            </template>
                            <UiKbdGroup v-else-if="settings?.hotkeys[action]">
                                <UiKbd
                                    v-for="key in settings.hotkeys[action]!.split('+')"
                                    :key="key"
                                >
                                    {{ KEY_LABELS[key] ?? key }}
                                </UiKbd>
                            </UiKbdGroup>
                            <span v-else class="text-muted-foreground">
                                {{ $t('hotkeys.notSet') }}
                            </span>
                        </UiButton>
                        <UiButton
                            variant="ghost"
                            size="icon-sm"
                            :aria-label="$t('hotkeys.clear')"
                            :disabled="!settings?.hotkeysEnabled || !settings?.hotkeys[action]"
                            @click="handleSet(action, '')"
                        >
                            <IX />
                        </UiButton>
                    </UiItemActions>
                </UiItem>
            </UiItemGroup>
        </UiCardContent>
        <UiCardFooter>
            <UiButton variant="outline" :disabled="!settings" @click="handleReset">
                <IRotateCcw />
                {{ $t('hotkeys.reset') }}
            </UiButton>
        </UiCardFooter>
    </UiCard>

    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('hotkeys.printScreen') }}</UiCardTitle>
            <UiCardDescription>{{ $t('hotkeys.printScreenDescription') }}</UiCardDescription>
            <UiCardAction>
                <UiSwitch
                    :model-value="!isSnippingToolEnabled"
                    :disabled="isSnippingToolEnabled === null"
                    :aria-label="$t('hotkeys.usePrintScreen')"
                    @update:model-value="handleSnippingTool"
                />
            </UiCardAction>
        </UiCardHeader>
    </UiCard>
</template>

<script setup lang="ts">
import type { HotkeyStatus } from '../../../shared/ipc';
import { DEFAULT_HOTKEYS, HOTKEY_ACTIONS, type HotkeyAction } from '../../../shared/settings';

const KEY_LABELS: Record<string, string> = {
    Control: 'Ctrl',
    Super: 'Win',
    PrintScreen: 'Print Screen',
    Up: '↑',
    Down: '↓',
    Left: '←',
    Right: '→',
};

const CODE_KEYS: Record<string, string> = {
    Space: 'Space',
    Enter: 'Enter',
    Tab: 'Tab',
    Backspace: 'Backspace',
    Delete: 'Delete',
    Insert: 'Insert',
    Home: 'Home',
    End: 'End',
    PageUp: 'PageUp',
    PageDown: 'PageDown',
    ArrowUp: 'Up',
    ArrowDown: 'Down',
    ArrowLeft: 'Left',
    ArrowRight: 'Right',
    PrintScreen: 'PrintScreen',
    Minus: '-',
    Equal: '=',
    BracketLeft: '[',
    BracketRight: ']',
    Backslash: '\\',
    Semicolon: ';',
    Quote: "'",
    Comma: ',',
    Period: '.',
    Slash: '/',
    Backquote: '`',
    NumpadAdd: 'numadd',
    NumpadSubtract: 'numsub',
    NumpadMultiply: 'nummult',
    NumpadDivide: 'numdiv',
    NumpadDecimal: 'numdec',
};

const MODIFIER_CODES = ['Control', 'Alt', 'Shift', 'Meta', 'OS'];

const settings = useSettings();
const { $toast } = useNuxtApp();

const status = ref<HotkeyStatus>({});
const recording = ref<HotkeyAction | null>(null);
const isSnippingToolEnabled = ref<boolean | null>(null);

const duplicates = computed(() => {
    const result = new Set<HotkeyAction>();
    const seen = new Map<string, HotkeyAction>();

    for (const action of HOTKEY_ACTIONS) {
        const accelerator = settings.value?.hotkeys[action]?.toLowerCase();
        if (!accelerator) continue;

        const other = seen.get(accelerator);

        if (other) {
            result.add(action);
            result.add(other);
        } else {
            seen.set(accelerator, action);
        }
    }

    return result;
});

const keyFromCode = (code: string) => {
    if (/^Key[A-Z]$/.test(code)) return code.slice(3);
    if (/^Digit\d$/.test(code)) return code.slice(5);
    if (/^Numpad\d$/.test(code)) return `num${code.slice(6)}`;
    if (/^F\d{1,2}$/.test(code)) return code;
    return CODE_KEYS[code] ?? null;
};

const toAccelerator = (event: KeyboardEvent) => {
    const key = keyFromCode(event.code);
    if (!key) return null;

    const modifiers = [
        event.ctrlKey && 'Control',
        event.altKey && 'Alt',
        event.shiftKey && 'Shift',
        event.metaKey && 'Super',
    ].filter(Boolean);

    return [...modifiers, key].join('+');
};

const loadStatus = async () => {
    status.value = await desktop('hotkeys:status');
};

const stopRecording = async () => {
    if (!recording.value) return;

    recording.value = null;
    status.value = await desktop('hotkeys:pause', false);
};

const handleRecord = async (action: HotkeyAction) => {
    if (recording.value === action) return stopRecording();

    recording.value = action;
    await desktop('hotkeys:pause', true);
};

const handleSet = async (action: HotkeyAction, accelerator: string) => {
    if (!settings.value) return;

    recording.value = null;
    await updateSettings({ hotkeys: { ...settings.value.hotkeys, [action]: accelerator } });
    await loadStatus();
};

const handleReset = async () => {
    await updateSettings({ hotkeys: { ...DEFAULT_HOTKEYS } });
    await loadStatus();
};

const handleToggle = async (enabled: boolean) => {
    await updateSettings({ hotkeysEnabled: enabled });
    await loadStatus();
};

const handleSnippingTool = async (useFileship: boolean) => {
    try {
        status.value = await desktop('hotkeys:setSnippingTool', !useFileship);
        isSnippingToolEnabled.value = !useFileship;
    } catch (error: any) {
        $toast.error(error.message);
    }
};

const handleKey = (event: KeyboardEvent) => {
    if (!recording.value) return;

    event.preventDefault();
    event.stopPropagation();

    if (MODIFIER_CODES.some((code) => event.code.startsWith(code))) return;

    if (event.code === 'Escape' && !event.ctrlKey && !event.altKey && !event.shiftKey) {
        stopRecording();
        return;
    }

    const accelerator = toAccelerator(event);
    if (accelerator) handleSet(recording.value, accelerator);
};

useEventListener('keydown', (event: KeyboardEvent) => {
    if (event.code !== 'PrintScreen') handleKey(event);
});

useEventListener('keyup', (event: KeyboardEvent) => {
    if (event.code === 'PrintScreen') handleKey(event);
});

useEventListener('blur', stopRecording);

onBeforeUnmount(stopRecording);

onMounted(async () => {
    const [, snippingTool] = await Promise.all([loadStatus(), desktop('hotkeys:snippingTool')]);
    isSnippingToolEnabled.value = snippingTool;
});
</script>
