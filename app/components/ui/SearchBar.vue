<template>
    <div
        class="gradient-wrapper"
        relative
        h12
        wfull
        rounded-xl
        :class="[
            aiEnabled
                ? 'p-0.4 border-transparent animate-[gradient-shift_0.5s_forwards] bg-[linear-gradient(-45deg,#520aeb_0%,#e81d72_8%,#f9c43c_17%,#00a7ff_25%,white_34%,var(--fs-muted-1)_40%,var(--fs-overlay-4)_45%,var(--fs-accent)_100%)] bg-[size:400%_200%]'
                : 'border border-dashed border-fs-overlay-4 focus-within:(border-fs-accent border-solid) motion-safe:transition-all',
        ]"
    >
        <div
            :class="[inputClass, aiEnabled ? 'rounded-10px' : 'rounded-xl']"
            relative
            hfull
            wfull
            flex
            items-center
            bg-fs-overlay-2
            motion-safe:transition-colors
        >
            <Icon
                name="heroicons:magnifying-glass-16-solid"
                absolute
                left-3
                z-10
                h6
                w6
                text-fs-muted-2
                translate-y-="1/2"
                top="1/2"
            />

            <input
                v-model="searchQuery"
                type="text"
                :placeholder="aiEnabled ? '' : placeholder"
                hfull
                wfull
                border-none
                bg-transparent
                px11
                outline-none
                motion-safe:transition-all
                placeholder-fs-muted-2
            />

            <div
                v-if="aiEnabled"
                ref="aiSuggestion"
                translate-y-="1/2"
                top="1/2"
                pointer-events-none
                absolute
                left-11
                select-none
                text-fs-muted-2
                op0
                :class="searchQuery.length && '!op0'"
            >
                {{ aiSuggestions[currentSuggestionIndex] }}
            </div>

            <UiButton
                v-if="aiAvailable"
                alignment="center"
                icon="heroicons:sparkles-solid"
                icon-size="20"
                :variant="aiEnabled ? 'accent' : 'onOverlay'"
                translate-y-="1/2"
                top="1/2"
                p0="!"
                absolute
                right-2
                z-10
                h8
                w8
                :loading
                :disabled="loading"
                @click="aiEnabled = !aiEnabled"
            />
            <UiSpinner
                v-else-if="loading"
                :size="20"
                translate-y-="1/2"
                top="1/2"
                absolute
                right-3
            />
        </div>
    </div>
</template>

<script setup lang="ts">
import gsap from 'gsap';
import SplitText from 'gsap/SplitText';

const { aiAvailable } = defineProps<{
    placeholder: string;
    inputClass?: unknown;
    aiAvailable?: boolean;
}>();

const searchQuery = defineModel<string>({ required: true });

const aiEnabled = defineModel<boolean>('aiEnabled', {
    default: false,
    required: false,
});

const loading = defineModel<boolean>('loading', {
    required: false,
    default: false,
});

const aiSuggestion = useTemplateRef('aiSuggestion');

const aiSuggestions = [
    'A bright red sports car',
    'Calm ocean at sunset',
    'A futuristic flying vehicle',
    'A delicious plate of pasta',
    'A falcon hunting moment',
    'A foggy forest path',
    'An ancient Roman ruin',
    'Night city skyline',
    'A cute cat portrait',
    'Abstract, colorful artwork',
];

const currentSuggestionIndex = ref(0);

const handleSuggestionAnimation = () => {
    const splitText = new SplitText(aiSuggestion.value, {
        type: 'words',
    });

    gsap.set(aiSuggestion.value, { opacity: 1 });

    if (usePreferredReducedMotion().value === 'no-preference') {
        gsap.from(splitText.words, {
            opacity: 0,
            x: -6,
            duration: 0.5,
            stagger: 0.1,
            ease: 'expo.out',
            filter: 'blur(0.125rem)',
        });
    }
};

let suggestionInterval: NodeJS.Timeout;
let firstAnimateTimeout: NodeJS.Timeout;

watch(aiEnabled, (value) => {
    clearInterval(suggestionInterval);
    clearTimeout(firstAnimateTimeout);

    if (value) {
        nextTick(() => (firstAnimateTimeout = setTimeout(handleSuggestionAnimation, 500)));
        suggestionInterval = setInterval(() => {
            currentSuggestionIndex.value =
                (currentSuggestionIndex.value + 1) % aiSuggestions.length;
            nextTick(handleSuggestionAnimation);
        }, 5_000);
    }
});

onMounted(() => {
    gsap.registerPlugin(SplitText);

    if (aiAvailable) aiEnabled.value = true;
});
</script>

<style>
@keyframes gradient-shift {
    0% {
        background-position: 0% 0%;
    }
    100% {
        background-position: 100% 100%;
    }
}
</style>
