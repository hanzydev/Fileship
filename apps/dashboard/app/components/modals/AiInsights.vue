<template>
    <UiModal
        v-model="isOpen"
        :background-props="{ 'data-ignore-modal-outer-click': true }"
        data-ignore-modal-outer-click
        z70="!"
        background-class="z60!"
        ring-0="!"
        bg-transparent="!"
        overflow-visible="!"
        p0="!"
        :close-on-outer-click="false"
        :closable="false"
        @outer-click="handleClose"
    >
        <div relative isolate overflow-visible p3>
            <div
                ref="aiShell"
                class="aiRainbowRing"
                will-change="height,transform,opacity"
                relative
                overflow-hidden
                rounded-19px
                p-3px
                :style="{
                    '--ai-rainbow-speed': aiEntering ? '0.6s' : '7s',
                }"
            >
                <div
                    ref="aiCard"
                    aria-label="AI Insights"
                    relative
                    z-1
                    overflow-hidden
                    rounded-2xl
                    bg-fs-overlay-1
                    ring="1 fs-overlay-4"
                >
                    <div ref="aiCardInner" relative z-10 p8 space-y-4>
                        <div flex="~ items-center justify-between gap-2">
                            <div flex="~ items-center gap-2">
                                <Icon
                                    name="heroicons:sparkles-solid"
                                    size="24"
                                    bg="[linear-gradient(96.58deg,color-mix(in_srgb,var(--fs-accent)_10%,white)_-100%,var(--fs-accent)_100%)]"
                                />
                                <h2 ref="aiTitle" font-semibold tracking-wide>AI Insights</h2>
                            </div>

                            <UiButton
                                variant="onOverlay"
                                alignment="center"
                                class="size-11 shrink-0 text-fs-muted-2 transition-colors !rounded-xl !p0 hover:text-white"
                                icon="lucide:x"
                                icon-size="24"
                                @click="handleClose"
                            />
                        </div>

                        <div v-if="caption" space-y-1>
                            <span text-sm text-fs-muted-3 font-bold uppercase>Caption</span>
                            <p
                                ref="aiCaptionText"
                                border="~ fs-overlay-3"
                                rounded-xl
                                bg-fs-overlay-2
                                p-4
                                text-fs-muted-1
                            >
                                {{ caption }}
                            </p>
                        </div>

                        <div v-if="piiDetected" space-y-1>
                            <span text-sm text-fs-muted-3 font-bold uppercase>
                                Sensitive Content Detected
                            </span>
                            <p
                                ref="aiPiiText"
                                border="~ red-500/30"
                                rounded-xl
                                bg="red-500/10"
                                p-4
                                text-red-100
                            >
                                {{ piiReasons.join(', ') }}
                            </p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    </UiModal>
</template>

<script setup lang="ts">
import { gsap } from 'gsap';
import SplitText from 'gsap/SplitText';

const {
    caption = null,
    piiDetected = false,
    piiReasons = [],
} = defineProps<{
    caption?: string | null;
    piiDetected?: boolean;
    piiReasons?: string[];
}>();

const isOpen = defineModel<boolean>({ required: true });
const reducedMotion = usePreferredReducedMotion();

const aiShell = useTemplateRef<HTMLDivElement>('aiShell');
const aiCardInner = useTemplateRef<HTMLDivElement>('aiCardInner');

const aiTitle = useTemplateRef<HTMLElement>('aiTitle');
const aiCaptionText = useTemplateRef<HTMLElement>('aiCaptionText');
const aiPiiText = useTemplateRef<HTMLElement>('aiPiiText');

const aiEntering = ref(false);
const closing = ref(false);

let introTl: gsap.core.Timeline | null = null;
let splits: SplitText[] = [];

const cleanup = () => {
    introTl?.kill();
    introTl = null;
    splits.forEach((s) => s.revert());
    splits = [];
};

const animateText = () => {
    if (reducedMotion.value !== 'no-preference') return;

    const elements = [aiTitle.value, aiCaptionText.value, aiPiiText.value].filter(
        Boolean,
    ) as HTMLElement[];

    for (const el of elements) {
        const split = new SplitText(el, { type: 'words' });
        splits.push(split);

        const wordCount = split.words?.length || 1;
        const each = Math.min(0.075, 0.8 / wordCount);

        gsap.set(el, { opacity: 1, visibility: 'visible' });
        gsap.fromTo(
            split.words,
            { opacity: 0, x: -6, filter: 'blur(0.25rem)' },
            {
                opacity: 1,
                x: 0,
                filter: 'blur(0px)',
                duration: 1,
                stagger: { each },
                ease: 'expo.out',
            },
        );
    }
};

const animateIn = async () => {
    await nextTick();

    const shell = aiShell.value;
    const inner = aiCardInner.value;
    if (!shell || !inner) return;

    cleanup();

    const textEls = [aiTitle.value, aiCaptionText.value, aiPiiText.value].filter(
        Boolean,
    ) as HTMLElement[];
    textEls.forEach((el) => gsap.set(el, { opacity: 0, visibility: 'hidden' }));

    gsap.set(shell, { '--ai-ring-opacity': 1 });
    aiEntering.value = true;

    window.setTimeout(() => (aiEntering.value = false), 1000);

    gsap.set(shell, {
        height: 0,
        opacity: 0,
        y: 20,
        scale: 0.96,
        filter: 'blur(10px)',
    });

    introTl = gsap.timeline({ defaults: { ease: 'power3.out' } });

    introTl
        .to(shell, {
            height: 'auto',
            opacity: 1,
            y: 0,
            scale: 1,
            filter: 'blur(0px)',
            duration: 0.65,
        })
        .add(animateText, 0.2)
        .to(
            shell,
            {
                '--ai-ring-opacity': 0,
                duration: 1,
                ease: 'power2.inOut',
            },
            '+=0.4',
        );
};

const animateOut = async () => {
    const shell = aiShell.value;
    if (!shell) return;

    cleanup();

    await gsap.to(shell, {
        opacity: 0,
        y: 10,
        scale: 0.98,
        filter: 'blur(8px)',
        height: 0,
        duration: 0.35,
        ease: 'power3.in',
    });
};

const handleClose = async () => {
    if (!isOpen.value || closing.value) return;

    closing.value = true;

    await animateOut();

    isOpen.value = false;
    closing.value = false;
};

watch(isOpen, async (open) => {
    if (open) await animateIn();
    else cleanup();
});

onMounted(() => {
    gsap.registerPlugin(SplitText);
});
</script>

<style scoped>
@property --ai-angle {
    syntax: '<angle>';
    initial-value: 0deg;
    inherits: false;
}

.aiRainbowRing::before {
    background: conic-gradient(
        from var(--ai-angle),
        #520aeb,
        #e81d72,
        #f9c43c,
        #00a7ff,
        #fff,
        #520aeb
    );

    -webkit-mask:
        linear-gradient(#fff 0 0) content-box,
        linear-gradient(#fff 0 0);
    mask:
        linear-gradient(#fff 0 0) content-box,
        linear-gradient(#fff 0 0);

    animation: aiRainbowSpin var(--ai-rainbow-speed, 7s) linear infinite;

    @apply p-3px rounded-inherit inset-0 content-empty absolute pointer-events-none z-0 opacity-[--ai-ring-opacity,0] [mask-composite:exclude] [-webkit-mask-composite:xor];
}

@keyframes aiRainbowSpin {
    from {
        --ai-angle: 0deg;
    }
    to {
        --ai-angle: 360deg;
    }
}

@media (prefers-reduced-motion: reduce) {
    .aiRainbowRing::before {
        animation: none;
        --ai-angle: 45deg;
    }
}
</style>
