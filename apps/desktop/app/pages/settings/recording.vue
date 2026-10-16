<template>
    <UiCard>
        <UiCardHeader>
            <UiCardTitle>{{ $t('recordingSettings.ffmpeg') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('recordingSettings.ffmpegDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent class="flex flex-col gap-4">
            <UiAlert v-if="ffmpeg?.download?.error" variant="destructive">
                <ICircleAlert />
                <UiAlertTitle>{{ $t('recordingSettings.downloadFailed') }}</UiAlertTitle>
                <UiAlertDescription>{{ ffmpeg.download.error }}</UiAlertDescription>
            </UiAlert>

            <div v-if="isDownloading" class="flex flex-col gap-2">
                <TextBlock variant="small">
                    {{
                        $t('recordingSettings.downloading', {
                            version: ffmpeg?.managedVersion,
                            progress: ffmpeg?.download?.progress,
                        })
                    }}
                </TextBlock>
                <UiProgress :model-value="ffmpeg?.download?.progress ?? 0" />
            </div>

            <dl
                v-else-if="ffmpeg?.isInstalled"
                class="grid grid-cols-[auto_1fr] gap-x-6 gap-y-2 text-sm"
            >
                <dt class="text-muted-foreground">{{ $t('recordingSettings.version') }}</dt>
                <dd class="truncate text-right">{{ ffmpeg.version }}</dd>
                <dt class="text-muted-foreground">{{ $t('recordingSettings.location') }}</dt>
                <dd class="truncate text-right" :title="ffmpeg.path ?? ''">
                    {{
                        ffmpeg.isManaged
                            ? $t('recordingSettings.downloadedByFileship')
                            : ffmpeg.path
                    }}
                </dd>
                <dt class="text-muted-foreground">{{ $t('recordingSettings.capture') }}</dt>
                <dd class="text-right">
                    {{
                        ffmpeg.ddagrab
                            ? $t('recordingSettings.captureBoth')
                            : $t('recordingSettings.captureGdiOnly')
                    }}
                </dd>
                <dt class="text-muted-foreground">{{ $t('recordingSettings.gpuEncoders') }}</dt>
                <dd class="flex flex-wrap justify-end gap-1">
                    <UiBadge v-for="encoder in gpuEncoders" :key="encoder" variant="secondary">
                        {{ encoder }}
                    </UiBadge>
                    <span v-if="!gpuEncoders.length" class="text-muted-foreground">
                        {{ $t('recordingSettings.none') }}
                    </span>
                </dd>
            </dl>

            <UiAlert v-else>
                <IInfo />
                <UiAlertTitle>{{ $t('recordingSettings.notInstalled') }}</UiAlertTitle>
                <UiAlertDescription>
                    {{ $t('recordingSettings.notInstalledDescription') }}
                </UiAlertDescription>
            </UiAlert>
        </UiCardContent>
        <UiCardFooter class="flex-wrap gap-2">
            <UiButton v-if="ffmpeg?.isManaged" :disabled="isDownloading" @click="handleDownload">
                <IDownload />
                {{
                    ffmpeg?.isInstalled
                        ? $t('recordingSettings.downloadAgain')
                        : $t('recordingSettings.downloadVersion', {
                              version: ffmpeg?.managedVersion ?? '',
                          })
                }}
            </UiButton>
            <UiButton variant="outline" :disabled="isDownloading" @click="handlePickFfmpeg">
                <IFolderOpen />
                {{ $t('recordingSettings.useOwn') }}
            </UiButton>
            <UiButton
                v-if="!ffmpeg?.isManaged"
                variant="ghost"
                @click="updateSection('recording', { ffmpegPath: '' })"
            >
                <IRotateCcw />
                {{ $t('recordingSettings.useDownloaded') }}
            </UiButton>
        </UiCardFooter>
    </UiCard>

    <UiCard v-if="settings">
        <UiCardHeader>
            <UiCardTitle>{{ $t('recordingSettings.video') }}</UiCardTitle>
            <UiCardDescription>{{ $t('recordingSettings.videoDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>{{ $t('recordingSettings.encoder') }}</UiFieldLabel>
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
                        <UiFieldLabel>{{ $t('recordingSettings.preset') }}</UiFieldLabel>
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
                                    {{
                                        recording.preset ||
                                        $t('recordingSettings.defaultPreset', {
                                            preset: presets.fallback,
                                        })
                                    }}
                                </UiSelectValue>
                            </UiSelectTrigger>
                            <UiSelectContent>
                                <UiSelectItem :value="DEFAULT_OPTION">
                                    {{
                                        $t('recordingSettings.defaultPreset', {
                                            preset: presets.fallback,
                                        })
                                    }}
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
                        <UiFieldDescription v-if="presets.description">
                            {{ $t(presets.description) }}
                        </UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>{{ $t('recordingSettings.rateControl') }}</UiFieldLabel>
                        <UiTabs
                            :model-value="recording.rateControl"
                            @update:model-value="
                                updateSection('recording', {
                                    rateControl: $event as 'quality' | 'bitrate',
                                })
                            "
                        >
                            <UiTabsList class="w-full">
                                <UiTabsTrigger value="quality">
                                    {{ $t('recordingSettings.quality') }}
                                </UiTabsTrigger>
                                <UiTabsTrigger value="bitrate">
                                    {{ $t('recordingSettings.bitrate') }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                    </UiField>

                    <UiField v-if="recording.rateControl === 'quality'">
                        <UiFieldLabel>{{ $t(qualityLabel) }}</UiFieldLabel>
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
                        <UiFieldDescription>
                            {{ $t('recordingSettings.qualityDescription') }}
                        </UiFieldDescription>
                    </UiField>

                    <UiField v-else>
                        <UiFieldLabel>{{ $t('recordingSettings.bitrateKbps') }}</UiFieldLabel>
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
                        <UiFieldLabel>{{ $t('recordingSettings.frameRate') }}</UiFieldLabel>
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
                        <UiFieldLabel>{{ $t('recordingSettings.container') }}</UiFieldLabel>
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
                        <UiFieldError v-if="containerError">
                            {{ $t('recordingSettings.webmVideo') }}
                        </UiFieldError>
                    </UiField>

                    <UiField class="sm:col-span-2">
                        <UiFieldLabel>{{ $t('recordingSettings.captureMethod') }}</UiFieldLabel>
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
                                    {{ $t(SOURCE_LABELS[source]) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                        <UiFieldDescription>
                            {{ $t('recordingSettings.captureMethodDescription') }}
                        </UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>{{ $t('recordingSettings.startDelay') }}</UiFieldLabel>
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
                        <UiFieldLabel>{{ $t('recordingSettings.stopAfter') }}</UiFieldLabel>
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
                        <UiFieldDescription>
                            {{ $t('recordingSettings.stopAfterDescription') }}
                        </UiFieldDescription>
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
                            {{ $t(field.label) }}
                        </UiFieldLabel>
                        <UiFieldDescription>{{ $t(field.description) }}</UiFieldDescription>
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
            <UiCardTitle>{{ $t('recordingSettings.audio') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('recordingSettings.audioDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField
                    orientation="horizontal"
                    class="has-[>[data-slot=field-content]]:items-center"
                >
                    <UiFieldContent>
                        <UiFieldLabel for="audio-microphone">
                            {{ $t('recordingSettings.microphone') }}
                        </UiFieldLabel>
                        <UiFieldDescription>
                            {{ $t('recordingSettings.microphoneDescription') }}
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
                            <UiFieldLabel>{{ $t('recordingSettings.device') }}</UiFieldLabel>
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
                                        {{ $t('recordingSettings.windowsDefault') }}
                                    </UiSelectItem>
                                    <UiSelectItem
                                        v-for="device in microphones"
                                        :key="device.deviceId"
                                        :value="device.deviceId"
                                    >
                                        {{
                                            device.label ||
                                            $t('recordingSettings.unnamedMicrophone')
                                        }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                        </UiField>

                        <UiField>
                            <UiFieldLabel>
                                {{ $t('recordingSettings.volume', { value: volumes.microphone }) }}
                            </UiFieldLabel>
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
                                {{ $t(field.label) }}
                            </UiFieldLabel>
                            <UiFieldDescription>{{ $t(field.description) }}</UiFieldDescription>
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
                        <UiFieldLabel for="audio-system">
                            {{ $t('recordingSettings.systemAudio') }}
                        </UiFieldLabel>
                        <UiFieldDescription>
                            {{ $t('recordingSettings.systemAudioDescription') }}
                        </UiFieldDescription>
                    </UiFieldContent>
                    <UiSwitch
                        id="audio-system"
                        :model-value="audio.system"
                        @update:model-value="updateSection('audio', { system: $event })"
                    />
                </UiField>

                <UiField v-if="audio.system">
                    <UiFieldLabel>
                        {{ $t('recordingSettings.volume', { value: volumes.system }) }}
                    </UiFieldLabel>
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
                        <UiFieldLabel>{{ $t('recordingSettings.audioTracks') }}</UiFieldLabel>
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
                                    {{ $t('recordingSettings.mixed') }}
                                </UiTabsTrigger>
                                <UiTabsTrigger value="separate" :disabled="!hasBothSources">
                                    {{ $t('recordingSettings.separate') }}
                                </UiTabsTrigger>
                                <UiTabsTrigger value="both" :disabled="!hasBothSources">
                                    {{ $t('recordingSettings.both') }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                        <UiFieldDescription>{{ $t(trackDescription) }}</UiFieldDescription>
                    </UiField>

                    <div class="grid grid-cols-1 gap-4 sm:grid-cols-3">
                        <UiField>
                            <UiFieldLabel>{{ $t('recordingSettings.codec') }}</UiFieldLabel>
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
                                {{ $t('recordingSettings.webmOpus') }}
                            </UiFieldDescription>
                        </UiField>

                        <UiField>
                            <UiFieldLabel>{{ $t('recordingSettings.bitrate') }}</UiFieldLabel>
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
                                        {{ $t('recordingSettings.kbps', { value: bitrate }) }}
                                    </UiSelectItem>
                                </UiSelectContent>
                            </UiSelect>
                        </UiField>

                        <UiField>
                            <UiFieldLabel>{{ $t('recordingSettings.syncOffset') }}</UiFieldLabel>
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
                                {{ $t('recordingSettings.syncOffsetDescription') }}
                            </UiFieldDescription>
                        </UiField>
                    </div>
                </template>
            </UiFieldGroup>
        </UiCardContent>
    </UiCard>

    <UiCard v-if="settings">
        <UiCardHeader>
            <UiCardTitle>{{ $t('recordingSettings.gifTitle') }}</UiCardTitle>
            <UiCardDescription>
                {{ $t('recordingSettings.gifDescription') }}
            </UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <div class="grid grid-cols-1 gap-4 sm:grid-cols-2">
                    <UiField>
                        <UiFieldLabel>{{ $t('recordingSettings.frameRate') }}</UiFieldLabel>
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
                        <UiFieldLabel>{{ $t('recordingSettings.maxWidth') }}</UiFieldLabel>
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
                        <UiFieldDescription>
                            {{ $t('recordingSettings.maxWidthDescription') }}
                        </UiFieldDescription>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>{{ $t('recordingSettings.dithering') }}</UiFieldLabel>
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
                                    {{ $t(DITHER_LABELS[dither]) }}
                                </UiSelectItem>
                            </UiSelectContent>
                        </UiSelect>
                    </UiField>

                    <UiField>
                        <UiFieldLabel>{{ $t('recordingSettings.palette') }}</UiFieldLabel>
                        <UiTabs
                            :model-value="gif.statsMode"
                            @update:model-value="
                                updateSection('gif', { statsMode: $event as 'diff' | 'full' })
                            "
                        >
                            <UiTabsList class="w-full">
                                <UiTabsTrigger value="diff">
                                    {{ $t('recordingSettings.movingParts') }}
                                </UiTabsTrigger>
                                <UiTabsTrigger value="full">
                                    {{ $t('recordingSettings.wholeFrame') }}
                                </UiTabsTrigger>
                            </UiTabsList>
                        </UiTabs>
                    </UiField>
                </div>

                <UiField
                    orientation="horizontal"
                    class="has-[>[data-slot=field-content]]:items-center"
                >
                    <UiFieldContent>
                        <UiFieldLabel for="gif-loop">
                            {{ $t('recordingSettings.loopForever') }}
                        </UiFieldLabel>
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
            <UiCardTitle>{{ $t('recordingSettings.advanced') }}</UiCardTitle>
            <UiCardDescription>{{ $t('recordingSettings.advancedDescription') }}</UiCardDescription>
        </UiCardHeader>
        <UiCardContent>
            <UiFieldGroup>
                <UiField>
                    <UiFieldLabel for="recording-input-args">
                        {{ $t('recordingSettings.inputArgs') }}
                    </UiFieldLabel>
                    <UiInput
                        id="recording-input-args"
                        v-model="inputArgs"
                        class="font-mono"
                        :placeholder="INPUT_ARGS_PLACEHOLDER"
                        @change="updateSection('recording', { inputArgs })"
                    />
                </UiField>
                <UiField>
                    <UiFieldLabel for="recording-output-args">
                        {{ $t('recordingSettings.outputArgs') }}
                    </UiFieldLabel>
                    <UiInput
                        id="recording-output-args"
                        v-model="outputArgs"
                        class="font-mono"
                        :placeholder="OUTPUT_ARGS_PLACEHOLDER"
                        @change="updateSection('recording', { outputArgs })"
                    />
                    <UiFieldDescription>
                        {{ $t('recordingSettings.outputArgsDescription') }}
                    </UiFieldDescription>
                </UiField>
                <UiField>
                    <UiFieldLabel for="recording-command">
                        {{ $t('recordingSettings.command') }}
                    </UiFieldLabel>
                    <UiTextarea
                        id="recording-command"
                        :model-value="preview"
                        readonly
                        class="min-h-32 font-mono text-xs"
                    />
                    <UiFieldDescription>
                        {{ $t('recordingSettings.commandDescription') }}
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
const INPUT_ARGS_PLACEHOLDER = '-rtbufsize 512M';
const OUTPUT_ARGS_PLACEHOLDER = '-g 120 -bf 0';

const SOURCE_LABELS: Record<(typeof VIDEO_SOURCES)[number], string> = {
    auto: 'recordingSettings.automatic',
    ddagrab: 'recordingSettings.ddagrab',
    gdigrab: 'recordingSettings.gdigrab',
};

const CODEC_LABELS: Record<Exclude<VideoCodec, 'auto'>, string> = {
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
    sierra2_4a: 'recordingSettings.ditherSierra',
    floyd_steinberg: 'recordingSettings.ditherFloydSteinberg',
    bayer: 'recordingSettings.ditherBayer',
    none: 'recordingSettings.none',
};

const VIDEO_SWITCHES = [
    {
        key: 'showCursor',
        label: 'recordingSettings.showCursor',
        description: 'recordingSettings.showCursorDescription',
    },
    {
        key: 'showBorder',
        label: 'recordingSettings.showBorder',
        description: 'recordingSettings.showBorderDescription',
    },
] as const;

const MICROPHONE_SWITCHES = [
    {
        key: 'noiseSuppression',
        label: 'recordingSettings.noiseSuppression',
        description: 'recordingSettings.noiseSuppressionDescription',
    },
    {
        key: 'echoCancellation',
        label: 'recordingSettings.echoCancellation',
        description: 'recordingSettings.echoCancellationDescription',
    },
    {
        key: 'autoGainControl',
        label: 'recordingSettings.autoGainControl',
        description: 'recordingSettings.autoGainControlDescription',
    },
] as const;

const PRESETS: { match: RegExp; options: string[]; description: string }[] = [
    {
        match: /^libx26[45]$/,
        options: ['ultrafast', 'superfast', 'veryfast', 'faster', 'fast', 'medium', 'slow'],
        description: 'recordingSettings.presetX26x',
    },
    {
        match: /_nvenc$/,
        options: ['p1', 'p2', 'p3', 'p4', 'p5', 'p6', 'p7'],
        description: 'recordingSettings.presetNvenc',
    },
    {
        match: /_amf$/,
        options: ['speed', 'balanced', 'quality'],
        description: 'recordingSettings.presetAmf',
    },
    {
        match: /_qsv$/,
        options: ['veryfast', 'faster', 'fast', 'medium', 'slow', 'slower', 'veryslow'],
        description: 'recordingSettings.presetQsv',
    },
    {
        match: /^libvpx-vp9$/,
        options: ['5', '6', '7', '8'],
        description: 'recordingSettings.presetVp9',
    },
    {
        match: /^libsvtav1$/,
        options: ['8', '9', '10', '11', '12', '13'],
        description: 'recordingSettings.presetSvtAv1',
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
const { t } = useI18n();

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
        fallback: DEFAULT_PRESETS[codec] ?? DEFAULT_PRESETS[key] ?? t('recordingSettings.none'),
    };
});

const qualityLabel = computed(() => {
    const codec = effectiveCodec.value;
    if (codec.endsWith('_nvenc')) return 'recordingSettings.constantQuality';
    if (codec.endsWith('_amf')) return 'recordingSettings.quantizer';
    if (codec.endsWith('_qsv')) return 'recordingSettings.globalQuality';
    return 'recordingSettings.constantRateFactor';
});

const containerError = computed(() => {
    if (recording.value.container !== 'webm') return false;

    const codec = effectiveCodec.value;
    return !(codec === 'libvpx-vp9' || codec === 'libsvtav1' || codec.startsWith('av1'));
});

const trackDescription = computed(() => {
    if (!hasBothSources.value) return 'recordingSettings.tracksSingle';
    if (audio.value.tracks === 'mixed') return 'recordingSettings.tracksMixed';
    if (audio.value.tracks === 'separate') return 'recordingSettings.tracksSeparate';

    return 'recordingSettings.tracksBoth';
});

const codecLabel = (codec: VideoCodec) =>
    codec === 'auto'
        ? t('recordingSettings.automaticCodec', {
              codec: CODEC_LABELS[effectiveCodec.value as Exclude<VideoCodec, 'auto'>],
          })
        : CODEC_LABELS[codec];

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
    } catch {
        $toast.error(t('recordingSettings.microphonesFailed'));
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
