<template>
    <div
        v-if="isBorder"
        class="fixed inset-0 border-dashed border-destructive"
        :class="{
            'border-t-2': sides.includes('t'),
            'border-r-2': sides.includes('r'),
            'border-b-2': sides.includes('b'),
            'border-l-2': sides.includes('l'),
        }"
    />

    <div v-else class="flex size-full items-center justify-center p-1 select-none">
        <div
            class="flex h-12 items-center gap-1 rounded-4xl border bg-popover px-1.5 text-popover-foreground shadow-md"
        >
            <div class="flex min-w-24 items-center gap-2 pl-2.5">
                <UiSpinner v-if="isBusy" />
                <IPause v-else-if="state.phase === 'paused'" class="size-4 text-muted-foreground" />
                <span v-else class="size-2.5 animate-pulse rounded-full bg-destructive" />

                <TextBlock variant="small" class="tabular-nums">{{ label }}</TextBlock>
            </div>

            <UiSeparator orientation="vertical" class="mx-1 my-3" />

            <UiButton
                v-for="source in sources"
                :key="source"
                variant="ghost"
                size="icon-sm"
                :aria-label="
                    source === 'microphone' ? $t('recorder.microphone') : $t('recorder.systemAudio')
                "
                :aria-pressed="!state.muted.includes(source)"
                :disabled="isBusy"
                @click="handleMute(source, !state.muted.includes(source))"
            >
                <template v-if="source === 'microphone'">
                    <IMic v-if="!state.muted.includes(source)" />
                    <IMicOff v-else class="text-destructive" />
                </template>
                <template v-else>
                    <IVolume2 v-if="!state.muted.includes(source)" />
                    <IVolumeX v-else class="text-destructive" />
                </template>
            </UiButton>

            <UiButton
                variant="ghost"
                size="icon-sm"
                :aria-label="
                    state.phase === 'paused' ? $t('recorder.resume') : $t('recorder.pause')
                "
                :disabled="state.phase !== 'recording' && state.phase !== 'paused'"
                @click="handleControl(state.phase === 'paused' ? 'resume' : 'pause')"
            >
                <IPlay v-if="state.phase === 'paused'" />
                <IPause v-else />
            </UiButton>
            <UiButton
                variant="ghost"
                size="icon-sm"
                :aria-label="$t('recorder.discard')"
                :disabled="state.phase === 'processing'"
                @click="handleControl('abort')"
            >
                <ITrash2 />
            </UiButton>
            <UiButton
                variant="destructive"
                size="sm"
                :disabled="isBusy"
                @click="handleControl('stop')"
            >
                <ISquare class="fill-current" />
                {{ $t('recorder.stop') }}
            </UiButton>
        </div>
    </div>
</template>

<script setup lang="ts">
import type { AudioSourceId, RecorderControl, RecorderState, Settings } from '../../shared/ipc';

const WORKLET = `
class FileshipPcm extends AudioWorkletProcessor {
    constructor() {
        super();
        this.frames = 2400;
        this.buffer = new Float32Array(this.frames * 2);
        this.length = 0;
    }

    process(inputs) {
        const input = inputs[0] || [];
        const left = input[0];
        const right = input[1] || left;
        const size = left ? left.length : 128;

        for (let i = 0; i < size; i++) {
            this.buffer[this.length++] = left ? left[i] : 0;
            this.buffer[this.length++] = right ? right[i] : 0;

            if (this.length === this.buffer.length) {
                this.port.postMessage(this.buffer.buffer, [this.buffer.buffer]);
                this.buffer = new Float32Array(this.frames * 2);
                this.length = 0;
            }
        }

        return true;
    }
}

registerProcessor('fileship-pcm', FileshipPcm);
`;

const SOURCE_ERRORS: Record<AudioSourceId, string> = {
    microphone: 'recorder.microphoneUnavailable',
    system: 'recorder.systemAudioUnavailable',
};

const route = useRoute();
const { t } = useI18n();
const isBorder = route.query.view === 'border';
const sides = typeof route.query.sides === 'string' ? route.query.sides : 'trbl';

const state = ref<RecorderState>({
    phase: 'preparing',
    kind: 'video',
    elapsed: 0,
    since: null,
    countdown: 0,
    tracks: [],
    muted: [],
    maxDuration: 0,
});

const now = useNow({ scheduler: (callback) => useIntervalFn(callback, 250) });
const streams: MediaStream[] = [];
const gains: Partial<Record<AudioSourceId, { node: GainNode; volume: number }>> = {};
let context: AudioContext | null = null;

const sources = computed(() => [...new Set(state.value.tracks.flatMap((track) => track.sources))]);

const isBusy = computed(() => ['preparing', 'countdown', 'processing'].includes(state.value.phase));

const formatTime = (milliseconds: number) => {
    const total = Math.floor(milliseconds / 1000);
    const hours = Math.floor(total / 3600);
    const minutes = Math.floor((total % 3600) / 60);
    const seconds = String(total % 60).padStart(2, '0');

    return hours
        ? `${hours}:${String(minutes).padStart(2, '0')}:${seconds}`
        : `${minutes}:${seconds}`;
};

const label = computed(() => {
    const { phase, countdown, elapsed, since, maxDuration } = state.value;

    if (phase === 'preparing') return t('recorder.starting');
    if (phase === 'countdown') return t('recorder.startingIn', { count: countdown });
    if (phase === 'processing') return t('recorder.processing');

    const time = formatTime(elapsed + (since ? now.value.getTime() - since : 0));
    return maxDuration ? `${time} / ${formatTime(maxDuration)}` : time;
});

const applyMute = () => {
    for (const [source, gain] of Object.entries(gains)) {
        const isMuted = state.value.muted.includes(source as AudioSourceId);
        gain!.node.gain.value = isMuted ? 0 : gain!.volume;
    }
};

const openMicrophone = async (audio: Settings['audio']) => {
    const constraints = {
        noiseSuppression: audio.noiseSuppression,
        echoCancellation: audio.echoCancellation,
        autoGainControl: audio.autoGainControl,
        channelCount: { ideal: 2 },
    };

    try {
        return await navigator.mediaDevices.getUserMedia({
            audio: {
                ...constraints,
                ...(audio.microphoneId ? { deviceId: { exact: audio.microphoneId } } : {}),
            },
        });
    } catch (error) {
        if (!audio.microphoneId) throw error;
        return navigator.mediaDevices.getUserMedia({ audio: constraints });
    }
};

const openSystemAudio = async () => {
    const stream = await navigator.mediaDevices.getDisplayMedia({
        video: true,
        audio: {
            echoCancellation: false,
            noiseSuppression: false,
            autoGainControl: false,
        },
    });

    for (const track of stream.getVideoTracks()) track.stop();
    if (!stream.getAudioTracks().length) throw new Error();

    return stream;
};

const startAudio = async () => {
    const needed = sources.value;
    if (!needed.length) return;

    const { audio } = await desktop('settings:get');

    context = new AudioContext({ sampleRate: 48_000, latencyHint: 'playback' });

    const module = URL.createObjectURL(new Blob([WORKLET], { type: 'text/javascript' }));
    await context.audioWorklet.addModule(module);
    URL.revokeObjectURL(module);

    const volumes: Record<AudioSourceId, number> = {
        microphone: audio.microphoneVolume / 100,
        system: audio.systemVolume / 100,
    };

    for (const source of needed) {
        const stream = await (
            source === 'microphone' ? openMicrophone(audio) : openSystemAudio()
        ).catch(() => {
            throw new Error(t(SOURCE_ERRORS[source]));
        });

        streams.push(stream);

        const node = context.createGain();
        context.createMediaStreamSource(stream).connect(node);
        gains[source] = { node, volume: volumes[source] };
    }

    applyMute();

    const sink = context.createGain();
    sink.gain.value = 0;
    sink.connect(context.destination);

    for (const [index, track] of state.value.tracks.entries()) {
        const tap = new AudioWorkletNode(context, 'fileship-pcm', {
            numberOfInputs: 1,
            numberOfOutputs: 1,
            outputChannelCount: [2],
            channelCount: 2,
            channelCountMode: 'explicit',
            channelInterpretation: 'speakers',
        });

        for (const source of track.sources) gains[source]!.node.connect(tap);
        tap.connect(sink);
        tap.port.onmessage = ({ data }) => window.fileship.invoke('recorder:audio', index, data);
    }

    await context.resume();
};

const handleControl = (action: RecorderControl) => desktop('recorder:control', action);

const handleMute = (source: AudioSourceId, muted: boolean) =>
    desktop('recorder:mute', source, muted);

useHead({
    htmlAttrs: { class: 'bg-transparent' },
    bodyAttrs: { class: 'bg-transparent overflow-hidden' },
    style: [{ innerHTML: '#nuxt-devtools-container { display: none !important; }' }],
});

const reveal = async () => {
    await nextTick();
    await new Promise((resolve) => requestAnimationFrame(() => requestAnimationFrame(resolve)));
    await desktop('recorder:visible', isBorder ? 'border' : 'toolbar');
};

onMounted(async () => {
    reveal();
    if (isBorder) return;

    const off = window.fileship.on('recorder:state', (value) => {
        state.value = value;
        applyMute();
    });

    onBeforeUnmount(() => {
        off();
        for (const stream of streams) for (const track of stream.getTracks()) track.stop();
        context?.close();
    });

    state.value = await desktop('recorder:get');

    try {
        await startAudio();
        await desktop('recorder:ready', null);
    } catch (error: any) {
        await desktop(
            'recorder:ready',
            error instanceof DOMException ? t('recorder.audioDevicesFailed') : error.message,
        );
    }
});

definePageMeta({
    layout: false,
});
</script>
