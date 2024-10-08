<template>
    <div>
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in)"
            leave-active-class="motion-safe:(animate-out fade-out)"
        >
            <div
                v-if="isOpen && width > 640 && width < 1024"
                absolute
                inset-0
                z20
                backdrop-blur-sm
                lg:ml64
                bg-black="/20"
                :style="{
                    height: `calc(100vh - 70px${adminSessionId ? ' - 3rem' : ''})`,
                    marginTop: `calc(70px${adminSessionId ? ' + 48px' : ''})`,
                }"
                @click="isOpen = false"
            />
        </Transition>
        <Transition
            enter-active-class="motion-safe:(animate-in fade-in slide-in-left-full)"
            leave-active-class="motion-safe:(animate-out fade-out slide-out-left-full)"
        >
            <aside
                v-if="width < 1024 ? isOpen : true"
                :class="width === Infinity && 'lt-lg:hidden'"
                data-allow-mismatch
                absolute
                z20
                flex="~ col gap2"
                wfull
                overflow-y-auto
                bg-fs-overlay-2
                pt0.5
                sm:w64
                lg:pt4
                :style="{
                    height: `calc(100vh - 70px${adminSessionId ? ' - 3rem' : ''})`,
                }"
            >
                <UiButton
                    v-for="item in items"
                    :key="item.name"
                    :href="item.href"
                    :variant="item.href === route.path ? 'accent' : 'primary'"
                    :icon="item.icon"
                    icon-size="20"
                    alignment="left"
                    mx5
                    gap2.5
                    px2.5
                    lg:mx3
                >
                    {{ item.name }}
                </UiButton>

                <a
                    href="https://github.com/hanzydev/Fileship"
                    target="_blank"
                    flex="~ items-center gap2.5"
                    m5.5
                    mta
                    wfit
                    text-slate300
                    font-medium
                    hover:text-white
                    motion-safe:transition-colors
                >
                    <Icon name="heroicons-solid:external-link" size="20" />
                    Github
                </a>
            </aside>
        </Transition>
    </div>
</template>

<script setup lang="ts">
defineProps<{
    items: { name: string; href: string; icon: string }[];
}>();

const isOpen = useSidebar();
const router = useRouter();
const route = useRoute();
const overflow = useOverflow();

const adminSessionId = useCookie('adminSessionId');

const { width } = useWindowSize();

onKeyStroke(
    'Escape',
    () => {
        if (isOpen.value) isOpen.value = false;
    },
    { eventName: 'keydown' },
);

onUnmounted(() => (overflow.value = true));

watch(isOpen, (value) => (overflow.value = !value));

watch(
    width,
    (value) => (overflow.value = value >= 1024 ? true : !isOpen.value),
);

router.beforeEach((_, __, next) => {
    if (isOpen.value) isOpen.value = false;
    next();
});
</script>
