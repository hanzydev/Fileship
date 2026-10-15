<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>ffmpeg</UiCardTitle>
            <UiCardDescription>
                Fileship records with ffmpeg. Download the tested build or point to your own.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-4">
            <UiAlert v-if="ffmpeg?.download?.error" variant="destructive">
                <ICircleAlert />
                <UiAlertTitle>Download failed</UiAlertTitle>
                <UiAlertDescription>{{ ffmpeg.download.error }}</UiAlertDescription>
            </UiAlert>

            <div v-if="isDownloading" class="flex flex-col gap-2">
                <TextBlock variant="small">
                    Downloading ffmpeg {{ ffmpeg?.managedVersion }}...
                    {{ ffmpeg?.download?.progress }}%
                </TextBlock>
                <UiProgress :model-value="ffmpeg?.download?.progress ?? 0" />
            </div>

            <dl
                v-else-if="ffmpeg?.isInstalled"
                class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm"
            >
                <dt class="text-muted-foreground">Version</dt>
                <dd class="truncate text-right">{{ ffmpeg.version }}</dd>
                <dt class="text-muted-foreground">Location</dt>
                <dd class="truncate text-right" :title="ffmpeg.path ?? ''">
                    {{ ffmpeg.isManaged ? 'Downloaded by Fileship' : ffmpeg.path }}
                </dd>
                <dt class="text-muted-foreground">Capture</dt>
                <dd class="text-right">
                    {{ ffmpeg.ddagrab ? 'Desktop Duplication and GDI' : 'GDI only' }}
                </dd>
                <dt class="text-muted-foreground">GPU encoders</dt>
                <dd class="flex flex-wrap justify-end gap-1">
                    <UiBadge v-for="encoder in gpuEncoders" :key="encoder" variant="secondary">
                        {{ encoder }}
                    </UiBadge>
                    <span v-if="!gpuEncoders.length" class="text-muted-foreground">None</span>
                </dd>
            </dl>

            <UiAlert v-else>
                <IInfo />
                <UiAlertTitle>ffmpeg is not installed</UiAlertTitle>
                <UiAlertDescription>
                    Screen and GIF recording need it. The download is about 110 MB.
                </UiAlertDescription>
            </UiAlert>
        </UiCardContent>
        <UiCardFooter class="flex-wrap gap-2">
            <UiButton v-if="ffmpeg?.isManaged" :disabled="isDownloading" @click="handleDownload">
                <IDownload />
                {{
                    ffmpeg?.isInstalled
                        ? 'Download again'
                        : `Download ffmpeg ${ffmpeg?.managedVersion ?? ''}`
                }}
            </UiButton>
            <UiButton variant="outline" :disabled="isDownloading" @click="handlePickFfmpeg">
                <IFolderOpen />
                Use my own ffmpeg.exe
            </UiButton>
            <UiButton
                v-if="!ffmpeg?.isManaged"
                variant="ghost"
                @click="updateSection('recording', { ffmpegPath: '' })"
            >
                <IRotateCcw />
                Use the downloaded one
            </UiButton>
        </UiCardFooter>
    </UiCard>

    <UiCard v-if="settings">
        <UiCardHeader>
            <UiCardTitle>Video</UiCardTitle>
            <UiCardDescription>How screen recordings are captured and encoded.</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>Encoder</UiFieldLabel>
                        <UiSelect
                            :model-value="recording.codec"
                            @update:model-value="handleCodec($event as VideoCodec)"
                        >
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue>{{ codecLabel(recording.codec) }}</UiSelectValue>
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="codec in VIDEO_CODECS"
                                    :key="codec"
                                    :value="codec"
                                    :disabled="!isAvailable(codec)"
                                >
                                    {{ codecLabel(codec) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Preset</UiFieldLabel>
                        <UiSelect
                            :model-value="recording.preset || DEFAULT_OPTION"
                            @update:model-value="
                                updateSection('recording', {
                                    preset: $event === DEFAULT_OPTION ? '' : ($event as string),
                                })
                            "
                        >
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue>
                                    {{ recording.preset || `Default (${presets.fallback})` }}
                                </UiSelectValue>
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem :value="DEFAULT_OPTION">
                                    Default ({{ presets.fallback }})
                                </UiSelectItem>
                                <UiSelectItem
                                    v-for="preset in presets.options"
                                    :key="preset"
                                    :value="preset"
                                >
                                    {{ preset }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                        <UiFieldDescription>{{ presets.description }}</UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Rate control</UiFieldLabel>
                        <UiTabs
                            :model-value="recording.rateControl"
                            @update:model-value="
                                updateSection('recording', {
                                    rateControl: $event as 'quality' | 'bitrate',
                                })
                            "
                        >
                            <UiTabsList class="w-full">
                                <UiTabsTrigger value="quality">Quality</UiTabsTrigger>
                                <UiTabsTrigger value="bitrate">Bitrate</UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                    </UiField>

                    <UiField v-if="recording.rateControl === 'quality'">
                        <UiFieldLabel>{{ qualityLabel }}</UiFieldLabel>
                        <UiNumberField
                            :model-value="recording.quality"
                            :min="0"
                            :max="63"
                            @update:model-value="handleNumber('recording', 'quality', $event, 23)"
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                        <UiFieldDescription>Lower is sharper and bigger.</UiFieldDescription>
                    </UiField>

                    <UiField v-else>
                        <UiFieldLabel>Bitrate (kbps)</UiFieldLabel>
                        <UiNumberField
                            :model-value="recording.bitrate"
                            :min="100"
                            :max="500000"
                            :step="500"
                            @update:model-value="handleNumber('recording', 'bitrate', $event, 8000)"
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Frame rate</UiFieldLabel>
                        <UiNumberField
                            :model-value="recording.fps"
                            :min="1"
                            :max="240"
                            @update:model-value="handleNumber('recording', 'fps', $event, 30)"
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Container</UiFieldLabel>
                        <UiTabs
                            :model-value="recording.container"
                            @update:model-value="
                                updateSection('recording', {
                                    container: $event as (typeof CONTAINERS)[number],
                                })
                            "
                        >
                            <UiTabsList class="w-full">
                                <UiTabsTrigger
                                    v-for="container in CONTAINERS"
                                    :key="container"
                                    :value="container"
                                >
                                    {{ container.toUpperCase() }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                        <UiFieldError v-if="containerError">{{ containerError }}</UiFieldError>
                    </UiField>

                    <UiField class="sm:col-span-2">
                        <UiFieldLabel>Capture method</UiFieldLabel>
                        <UiSelect
                            :model-value="recording.source"
                            @update:model-value="
                                updateSection('recording', {
                                    source: $event as (typeof VIDEO_SOURCES)[number],
                                })
                            "
                        >
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="source in VIDEO_SOURCES"
                                    :key="source"
                                    :value="source"
                                >
                                    {{ SOURCE_LABELS[source] }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                        <UiFieldDescription>
                            Desktop Duplication runs on the GPU and keeps up with high frame rates.
                            Areas that span several monitors always use GDI.
                        </UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Start delay (seconds)</UiFieldLabel>
                        <UiNumberField
                            :model-value="recording.startDelay"
                            :min="0"
                            :max="30"
                            @update:model-value="handleNumber('recording', 'startDelay', $event, 0)"
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Stop after (seconds)</UiFieldLabel>
                        <UiNumberField
                            :model-value="recording.maxDuration"
                            :min="0"
                            :max="86400"
                            @update:model-value="
                                handleNumber('recording', 'maxDuration', $event, 0)
                            "
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                        <UiFieldDescription>0 records until you stop.</UiFieldDescription>
                    </UiField>
                </div>

                <UiFieldSeparator />

                <UiField
                    v-for="field in VIDEO_SWITCHES"
                    :key="field.key"
                    orientation="horizontal"
                    class="has-[>[data-slot=field-content]]:items-center"
                >
                    <UiFieldContent>
                        <UiFieldLabel :for="`recording-${field.key}`">
                            {{ field.label }}
                        </UiFieldLabel>
                        <UiFieldDescription>{{ field.description }}</UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        :id="`recording-${field.key}`"
                        :model-value="recording[field.key]"
                        @update:model-value="updateSection('recording', { [field.key]: $event })"
                    />
                </UiField>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>

    <UiCard v-if="settings">
        <UiCardHeader>
            <UiCardTitle>Audio</UiCardTitle>
            <UiCardDescription>
                Record your microphone, the sound your computer plays, or both at once.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField
                    orientation="horizontal"
                    class="has-[>[data-slot=field-content]]:items-center"
                >
                    <UiFieldContent>
                        <UiFieldLabel for="audio-microphone">Microphone</UiFieldLabel>
                        <UiFieldDescription>
                            Your voice or any other input device.
                        </UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        id="audio-microphone"
                        :model-value="audio.microphone"
                        @update:model-value="updateSection('audio', { microphone: $event })"
                    />
                </UiField>

                <template v-if="audio.microphone">
                    <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                        <UiField>
                            <UiFieldLabel>Device</UiFieldLabel>
                            <UiSelect
                                :model-value="audio.microphoneId || DEFAULT_OPTION"
                                @update:model-value="
                                    updateSection('audio', {
                                        microphoneId:
                                            $event === DEFAULT_OPTION ? '' : ($event as string),
                                    })
                                "
                            >
                                <UiSelectTrigger class="w-full">
                                    <UiSelectValue />
                                </UiSelectTrigger>
                                <UiSelectContent>
                                    <UiSelectItem :value="DEFAULT_OPTION">
                                        Windows default
                                    </UiSelectItem>
                                    <UiSelectItem
                                        v-for="device in microphones"
                                        :key="device.deviceId"
                                        :value="device.deviceId"
                                    >
                                        {{ device.label || 'Unnamed microphone' }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                        </UiField>

                        <UiField>
                            <UiFieldLabel>Volume · {{ volumes.microphone }}%</UiFieldLabel>
                            <UiSlider
                                :model-value="[volumes.microphone]"
                                :min="0"
                                :max="300"
                                :step="5"
                                class="h-9"
                                @update:model-value="volumes.microphone = $event?.[0] ?? 100"
                                @value-commit="
                                    updateSection('audio', { microphoneVolume: $event[0]! })
                                "
                            />
                        </UiField>
                    </div>

                    <UiField
                        v-for="field in MICROPHONE_SWITCHES"
                        :key="field.key"
                        orientation="horizontal"
                        class="has-[>[data-slot=field-content]]:items-center"
                    >
                        <UiFieldContent>
                            <UiFieldLabel :for="`audio-${field.key}`">
                                {{ field.label }}
                            </UiFieldLabel>
                            <UiFieldDescription>{{ field.description }}</UiFieldDescription>
                        </UiFieldContent>
                        <UiSwitch
                            :id="`audio-${field.key}`"
                            :model-value="audio[field.key]"
                            @update:model-value="updateSection('audio', { [field.key]: $event })"
                        />
                    </UiField>
                </template>

                <UiFieldSeparator />

                <UiField
                    orientation="horizontal"
                    class="has-[>[data-slot=field-content]]:items-center"
                >
                    <UiFieldContent>
                        <UiFieldLabel for="audio-system">System audio</UiFieldLabel>
                        <UiFieldDescription>
                            Everything you hear: videos, games, calls and notifications.
                        </UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        id="audio-system"
                        :model-value="audio.system"
                        @update:model-value="updateSection('audio', { system: $event })"
                    />
                </UiField>

                <UiField v-if="audio.system">
                    <UiFieldLabel>Volume · {{ volumes.system }}%</UiFieldLabel>
                    <UiSlider
                        :model-value="[volumes.system]"
                        :min="0"
                        :max="300"
                        :step="5"
                        class="h-9"
                        @update:model-value="volumes.system = $event?.[0] ?? 100"
                        @value-commit="updateSection('audio', { systemVolume: $event[0]! })"
                    />
                </UiField>

                <template v-if="audio.microphone || audio.system">
                    <UiFieldSeparator />

                    <UiField>
                        <UiFieldLabel>Audio tracks</UiFieldLabel>
                        <UiTabs
                            :model-value="audio.tracks"
                            @update:model-value="
                                updateSection('audio', {
                                    tracks: $event as (typeof AUDIO_TRACKS)[number],
                                })
                            "
                        >
                            <UiTabsList class="w-full">
                                <UiTabsTrigger value="mixed" :disabled="!hasBothSources">
                                    Mixed
                                </UiTabsTrigger>
                                <UiTabsTrigger value="separate" :disabled="!hasBothSources">
                                    Separate
                                </UiTabsTrigger>
                                <UiTabsTrigger value="both" :disabled="!hasBothSources">
                                    Both
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                        <UiFieldDescription>{{ trackDescription }}</UiFieldDescription>
                    </UiField>

                    <div class="grid grid-cols-1 gap-4 sm:grid-cols-3">
                        <UiField>
                            <UiFieldLabel>Codec</UiFieldLabel>
                            <UiSelect
                                :model-value="audio.codec"
                                :disabled="recording.container === 'webm'"
                                @update:model-value="
                                    updateSection('audio', {
                                        codec: $event as (typeof AUDIO_CODECS)[number],
                                    })
                                "
                            >
                                <UiSelectTrigger class="w-full">
                                    <UiSelectValue />
                                </UiSelectTrigger>
                                <UiSelectContent>
                                    <UiSelectItem
                                        v-for="codec in AUDIO_CODECS"
                                        :key="codec"
                                        :value="codec"
                                    >
                                        {{ AUDIO_CODEC_LABELS[codec] }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                            <UiFieldDescription v-if="recording.container === 'webm'">
                                WebM always uses Opus.
                            </UiFieldDescription>
                        </UiField>

                        <UiField>
                            <UiFieldLabel>Bitrate</UiFieldLabel>
                            <UiSelect
                                :model-value="audio.bitrate"
                                @update:model-value="
                                    updateSection('audio', { bitrate: $event as number })
                                "
                            >
                                <UiSelectTrigger class="w-full">
                                    <UiSelectValue />
                                </UiSelectTrigger>
                                <UiSelectContent>
                                    <UiSelectItem
                                        v-for="bitrate in AUDIO_BITRATES"
                                        :key="bitrate"
                                        :value="bitrate"
                                    >
                                        {{ bitrate }} kbps
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                        </UiField>

                        <UiField>
                            <UiFieldLabel>Sync offset (ms)</UiFieldLabel>
                            <UiNumberField
                                :model-value="audio.delay"
                                :min="-2000"
                                :max="2000"
                                :step="10"
                                @update:model-value="handleNumber('audio', 'delay', $event, 0)"
                            >
                                <UiNumberFieldContent>
                                    <UiNumberFieldDecrement />
                                    <UiNumberFieldInput />
                                    <UiNumberFieldIncrement />
                                </UiNumberFieldContent>
                            </UiNumberField>
                            <UiFieldDescription>
                                Positive values delay the sound.
                            </UiFieldDescription>
                        </UiField>
                    </div>
                </template>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>

    <UiCard v-if="settings">
        <UiCardHeader>
            <UiCardTitle>GIF</UiCardTitle>
            <UiCardDescription>
                GIFs are recorded losslessly first, then converted with an optimized palette.
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>Frame rate</UiFieldLabel>
                        <UiNumberField
                            :model-value="gif.fps"
                            :min="1"
                            :max="50"
                            @update:model-value="handleNumber('gif', 'fps', $event, 15)"
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Maximum width</UiFieldLabel>
                        <UiNumberField
                            :model-value="gif.maxWidth"
                            :min="0"
                            :max="7680"
                            :step="10"
                            @update:model-value="handleNumber('gif', 'maxWidth', $event, 0)"
                        >
                            <UiNumberFieldContent>
                                <UiNumberFieldDecrement />
                                <UiNumberFieldInput />
                                <UiNumberFieldIncrement />
                            </UiNumberFieldContent>
                        </UiNumberField>
                        <UiFieldDescription>0 keeps the recorded size.</UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Dithering</UiFieldLabel>
                        <UiSelect
                            :model-value="gif.dither"
                            @update:model-value="
                                updateSection('gif', {
                                    dither: $event as (typeof GIF_DITHERS)[number],
                                })
                            "
                        >
                            <UiSelectTrigger class="w-full">
                                <UiSelectValue />
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem
                                    v-for="dither in GIF_DITHERS"
                                    :key="dither"
                                    :value="dither"
                                >
                                    {{ DITHER_LABELS[dither] }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>Palette</UiFieldLabel>
                        <UiTabs
                            :model-value="gif.statsMode"
                            @update:model-value="
                                updateSection('gif', { statsMode: $event as 'diff' | 'full' })
                            "
                        >
                            <UiTabsList class="w-full">
                                <UiTabsTrigger value="diff">Moving parts</UiTabsTrigger>
                                <UiTabsTrigger value="full">Whole frame</UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                    </UiField>
                </div>

                <UiField
                    orientation="horizontal"
                    class="has-[>[data-slot=field-content]]:items-center"
                >
                    <UiFieldContent>
                        <UiFieldLabel for="gif-loop">Loop forever</UiFieldLabel>
                    </UiFieldContent>
                    <UiSwitch
                        id="gif-loop"
                        :model-value="gif.loop"
                        @update:model-value="updateSection('gif', { loop: $event })"
                    />
                </UiField>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>

    <UiCard v-if="settings">
        <UiCardHeader>
            <UiCardTitle>Advanced</UiCardTitle>
            <UiCardDescription>Extra ffmpeg arguments and the full command.</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField>
                    <UiFieldLabel for="recording-input-args">Input arguments</UiFieldLabel>
                    <UiInput
                        id="recording-input-args"
                        v-model="inputArgs"
                        class="font-mono"
                        placeholder="-rtbufsize 512M"
                        @change="updateSection('recording', { inputArgs })"
                    />
                </UiField>
                <UiField>
                    <UiFieldLabel for="recording-output-args">Output arguments</UiFieldLabel>
                    <UiInput
                        id="recording-output-args"
                        v-model="outputArgs"
                        class="font-mono"
                        placeholder="-g 120 -bf 0"
                        @change="updateSection('recording', { outputArgs })"
                    />
                    <UiFieldDescription>
                        Added before the output file of every recording.
                    </UiFieldDescription>
                </UiField>
                <UiField>
                    <UiFieldLabel for="recording-command">Command</UiFieldLabel>
                    <UiTextarea
                        id="recording-command"
                        :model-value="preview"
                        readonly
                        class="min-h-32 font-mono text-xs"
                    />
                    <UiFieldDescription>
                        Recorded for the primary monitor with your current settings.
                    </UiFieldDescription>
                </UiField>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>
</template>

<script setup lang="ts">
import type { FfmpegStatus } from '../../../shared/ipc';
import {
    AUDIO_CODECS,
    AUDIO_TRACKS,
    CONTAINERS,
    GIF_DITHERS,
    VIDEO_CODECS,
    VIDEO_SOURCES,
    type VideoCodec,
} from '../../../shared/settings';

type NumberSection = 'recording' | 'audio' | 'gif';

const DEFAULT_OPTION = 'default';
const AUDIO_BITRATES = [64, 96, 128, 160, 192, 256, 320];

const SOURCE_LABELS: Record<(typeof VIDEO_SOURCES)[number], string> = {
    auto: 'Automatic',
    ddagrab: 'Desktop Duplication',
    gdigrab: 'GDI',
};

const CODEC_LABELS: Record<VideoCodec, string> = {
    auto: 'Automatic',
    libx264: 'H.264 · x264 (CPU)',
    libx265: 'HEVC · x265 (CPU)',
    h264_nvenc: 'H.264 · NVIDIA NVENC',
    hevc_nvenc: 'HEVC · NVIDIA NVENC',
    av1_nvenc: 'AV1 · NVIDIA NVENC',
    h264_amf: 'H.264 · AMD AMF',
    hevc_amf: 'HEVC · AMD AMF',
    av1_amf: 'AV1 · AMD AMF',
    h264_qsv: 'H.264 · Intel Quick Sync',
    hevc_qsv: 'HEVC · Intel Quick Sync',
    av1_qsv: 'AV1 · Intel Quick Sync',
    'libvpx-vp9': 'VP9 · libvpx (CPU)',
    libsvtav1: 'AV1 · SVT-AV1 (CPU)',
};

const AUDIO_CODEC_LABELS: Record<(typeof AUDIO_CODECS)[number], string> = {
    aac: 'AAC',
    libopus: 'Opus',
    libmp3lame: 'MP3',
};

const DITHER_LABELS: Record<(typeof GIF_DITHERS)[number], string> = {
    sierra2_4a: 'Sierra (smooth)',
    floyd_steinberg: 'Floyd-Steinberg',
    bayer: 'Bayer (small files)',
    none: 'None',
};

const VIDEO_SWITCHES = [
    {
        key: 'showCursor',
        label: 'Record the mouse cursor',
        description: 'Draw the cursor into recordings.',
    },
    {
        key: 'showBorder',
        label: 'Show a border around the area',
        description: 'The border and the controls never appear in the recording.',
    },
] as const;

const MICROPHONE_SWITCHES = [
    {
        key: 'noiseSuppression',
        label: 'Noise suppression',
        description: 'Filter out fans, keyboards and background hum.',
    },
    {
        key: 'echoCancellation',
        label: 'Echo cancellation',
        description: 'Remove your speakers from the microphone when you do not use headphones.',
    },
    {
        key: 'autoGainControl',
        label: 'Automatic gain',
        description: 'Keep your voice at a steady level.',
    },
] as const;

const PRESETS: { match: RegExp; options: string[]; description: string }[] = [
    {
        match: /^libx26[45]$/,
        options: ['ultrafast', 'superfast', 'veryfast', 'faster', 'fast', 'medium', 'slow'],
        description: 'Faster presets use less CPU and make bigger files.',
    },
    {
        match: /_nvenc$/,
        options: ['p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7'],
        description: 'p1 is the fastest, p7 the best quality.',
    },
    {
        match: /_amf$/,
        options: ['speed', 'balanced', 'quality'],
        description: 'How much the GPU works for quality.',
    },
    {
        match: /_qsv$/,
        options: ['veryfast', 'faster', 'fast', 'medium', 'slow', 'slower', 'veryslow'],
        description: 'Faster presets keep the GPU free for other work.',
    },
    {
        match: /^libvpx-vp9$/,
        options: ['5', '6', '7', '8'],
        description: 'CPU usage level. Higher is faster.',
    },
    {
        match: /^libsvtav1$/,
        options: ['8', '9', '10', '11', '12', '13'],
        description: 'Higher is faster.',
    },
];

const DEFAULT_PRESETS: Record<string, string> = {
    libx264: 'veryfast',
    libx265: 'veryfast',
    nvenc: 'p4',
    amf: 'balanced',
    qsv: 'veryfast',
    'libvpx-vp9': '8',
    libsvtav1: '10',
};

const settings = useSettings();
const { $toast } = useNuxtApp();

const ffmpeg = ref<FfmpegStatus | null>(null);
const preview = ref('');
const microphones = ref<MediaDeviceInfo[]>([]);
const inputArgs = ref(settings.value?.recording.inputArgs ?? '');
const outputArgs = ref(settings.value?.recording.outputArgs ?? '');
const volumes = reactive({
    microphone: settings.value?.audio.microphoneVolume ?? 100,
    system: settings.value?.audio.systemVolume ?? 100,
});

const recording = computed(() => settings.value!.recording);
const audio = computed(() => settings.value!.audio);
const gif = computed(() => settings.value!.gif);

const isDownloading = computed(() => !!ffmpeg.value?.download && !ffmpeg.value.download.error);
const gpuEncoders = computed(
    () => ffmpeg.value?.encoders.filter((name) => /_(nvenc|amf|qsv)$/.test(name)) ?? [],
);
const hasBothSources = computed(() => audio.value.microphone && audio.value.system);

const effectiveCodec = computed(() => {
    if (recording.value.codec !== 'auto') return recording.value.codec;
    return (
        (['h264_nvenc', 'h264_amf', 'h264_qsv'] as const).find((codec) =>
            ffmpeg.value?.encoders.includes(codec),
        ) ?? 'libx264'
    );
});

const presets = computed(() => {
    const codec = effectiveCodec.value;
    const family = PRESETS.find((item) => item.match.test(codec));
    const key = codec.split('_').at(-1)!;

    return {
        options: family?.options ?? [],
        description: family?.description ?? '',
        fallback: DEFAULT_PRESETS[codec] ?? DEFAULT_PRESETS[key] ?? 'none',
    };
});

const qualityLabel = computed(() => {
    const codec = effectiveCodec.value;
    if (codec.endsWith('_nvenc')) return 'Constant quality (CQ)';
    if (codec.endsWith('_amf')) return 'Quantizer (QP)';
    if (codec.endsWith('_qsv')) return 'Global quality';
    return 'Constant rate factor (CRF)';
});

const containerError = computed(() => {
    if (recording.value.container !== 'webm') return null;

    const codec = effectiveCodec.value;
    if (codec === 'libvpx-vp9' || codec === 'libsvtav1' || codec.startsWith('av1')) return null;
    return 'WebM only holds VP9 or AV1 video. Pick one of those encoders.';
});

const trackDescription = computed(() => {
    if (!hasBothSources.value) return 'One track with the source you turned on.';
    if (audio.value.tracks === 'mixed')
        return 'One track with your microphone and system audio together.';
    if (audio.value.tracks === 'separate') {
        return 'Your microphone and system audio in their own tracks, so you can edit them apart.';
    }

    return 'A mixed track for playback, plus one track per source for editing.';
});

const codecLabel = (codec: VideoCodec) =>
    codec === 'auto' ? `Automatic (${CODEC_LABELS[effectiveCodec.value]})` : CODEC_LABELS[codec];

const isAvailable = (codec: VideoCodec) =>
    codec === 'auto' || !ffmpeg.value?.isInstalled || ffmpeg.value.encoders.includes(codec);

const handleCodec = (codec: VideoCodec) => updateSection('recording', { codec, preset: '' });

const handleNumber = (
    section: NumberSection,
    key: string,
    value: number | undefined,
    fallback: number,
) => updateSection(section, { [key]: Number.isFinite(value) ? Math.round(value!) : fallback });

const handleDownload = () => desktop('ffmpeg:download');

const handlePickFfmpeg = async () => {
    const path = await desktop('dialog:pickFfmpeg');
    if (path) updateSection('recording', { ffmpegPath: path });
};

const loadMicrophones = async () => {
    try {
        const devices = await navigator.mediaDevices.enumerateDevices();
        microphones.value = devices.filter(
            (device) => device.kind === 'audioinput' && device.deviceId !== 'default',
        );
    } catch (error: any) {
        $toast.error(error.message);
    }
};

watch(
    settings,
    async () => {
        preview.value = await desktop('recording:preview').catch(() => '');
    },
    { deep: true, immediate: true },
);

watch(
    () => [settings.value?.audio.microphoneVolume, settings.value?.audio.systemVolume],
    ([microphone, system]) => {
        volumes.microphone = microphone ?? 100;
        volumes.system = system ?? 100;
    },
);

useEventListener(
    import.meta.client ? navigator.mediaDevices : undefined,
    'devicechange',
    loadMicrophones,
);

onMounted(async () => {
    const off = window.fileship.on('ffmpeg:status', async (value) => {
        ffmpeg.value = value;
        preview.value = await desktop('recording:preview').catch(() => '');
    });

    onBeforeUnmount(off);

    ffmpeg.value = await desktop('ffmpeg:status');
    loadMicrophones();
});
</script>
