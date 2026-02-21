<template>
    <div
        relative
        isolate
        h12
        wfull
        rounded-xl
        transition-all
        duration-200
        ease-out
        :class="[
            'searchBarContainer',
            aiEnabled && 'ai bg-fs-overlay-2 border-transparent outline-2 outline-white/30',
            'border border-dashed border-fs-overlay-4 focus-within:(border-fs-accent border-solid)',
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
                name="solar:magnifer-linear"
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
                class="aiButton"
                motion-safe="transition-all ease-out duration-200"
                :class="[
                    aiToggleClass,
                    aiEnabled &&
                        '!text-white !bg-[linear-gradient(96.58deg,color-mix(in_srgb,var(--fs-accent)_80%,white)_-100%,var(--fs-accent)_100%)]',
                ]"
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

const { aiAvailable: _aiAvailable, aiToggleClass } = defineProps<{
    placeholder: string;
    inputClass?: unknown;
    aiAvailable?: boolean;
    aiToggleClass?: unknown;
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

const runtimeConfig = useRuntimeConfig();

const aiAvailable = computed(() => _aiAvailable && runtimeConfig.public.aiEnabled);

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

    if (aiAvailable.value) aiEnabled.value = true;
});
</script>

<style scoped>
.searchBarContainer::after {
    background: linear-gradient(
        -45deg,
        #520aeb 0%,
        #e81d72 8%,
        #f9c43c 17%,
        #00a7ff 25%,
        #fff 34%,
        var(--fs-muted-1) 40%,
        var(--fs-overlay-4) 45%,
        var(--fs-accent) 100%
    );
    transition:
        background-position 0.5s ease-in-out,
        opacity 0.1s ease-in-out;
    @apply content-empty absolute pointer-events-none -inset-2px -z-1 opacity-0 rounded-inherit bg-[size:400%_200%] bg-[position:0%_0%] [animation-fill-mode:backwards];
}

.searchBarContainer.ai::after {
    @apply bg-[position:100%_100%] op100;
}
</style>
