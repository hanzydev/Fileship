<template>
    <div>
        <Head>
            <Title>Backups</Title>
        </Head>

        <div space-y-6>
            <h2>Backups</h2>
            <div grid="~ 2xl:cols-5 lg:cols-3 md:cols-2 xl:cols-4 gap6">
                <New h132px :disabled="creating" @action="handleCreate" />
                <div relative overflow-hidden active:scale-95 motion-safe:transition-transform>
                    <New h132px icon="heroicons-solid:upload" :disabled="uploading" />
                    <input
                        :key="uploading.toString()"
                        absolute
                        inset-0
                        z10
                        hfull
                        wfull
                        op0
                        :class="uploading ? 'cursor-not-allowed' : 'cursor-pointer'"
                        type="file"
                        accept=".tgz"
                        :disabled="uploading"
                        @change.stop.prevent="handleLoad"
                    />
                    <Transition
                        enter-active-class="motion-safe:(animate-in fade-in slide-in-left animate-duration-250)"
                        leave-active-class="motion-safe:(animate-out fade-out slide-out-right animate-duration-250)"
                    >
                        <div
                            v-if="uploadProgress > 0"
                            absolute
                            bottom-0
                            left-0
                            h-2
                            bg-fs-accent
                            motion-safe:transition-width
                            :style="{
                                width: `${uploadProgress}%`,
                            }"
                            :class="uploadProgress < 100 ? 'rounded-bl-lg' : 'rounded-b-lg'"
                        ></div>
                    </Transition>
                </div>

                <template v-if="isLoading">
                    <UiSkeletonCard
                        v-for="i in randomNumber(3, 7)"
                        :key="i"
                        h132px
                        flex="~ col gap4 justify-between"
                    >
                        <UiSkeletonLine h5 wfull />
                        <div text-fs-muted-2 space-y-2>
                            <UiSkeletonLine h4 w16 />
                            <UiSkeletonLine h4 w40 />
                        </div>
                    </UiSkeletonCard>
                </template>
                <TransitionGroup v-else :css="false" @enter="enter" @leave="leave">
                    <div v-for="backup in backups" :key="backup.id" op0 class="backupCard">
                        <BackupCard :data="backup" />
                    </div>
                </TransitionGroup>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
const backups = useBackups();
const { $toast } = useNuxtApp();

const creating = ref(false);
const uploading = ref(false);
const uploadProgress = ref(0);

const isLoading = ref(!backups.value.length);

let {
    public: { fileChunkSize },
} = useRuntimeConfig();

fileChunkSize = (+fileChunkSize || 25) * 1024 * 1024;

const handleCreate = async () => {
    creating.value = true;

    try {
        await $fetch('/api/users/@me/backups', { method: 'POST' });
        $toast.success('Backup is being created, this may take a while');
    } catch (error: any) {
        $toast.error(error.data.message);
    }

    creating.value = false;
};

const handleLoad = async (event: Event) => {
    const files = Array.from((event.target as HTMLInputElement).files!);
    const file = files[0];

    if (!file?.name.endsWith('.tgz')) {
        return $toast.error('Invalid backup file');
    }

    uploading.value = true;

    $toast.info('Upload in progress...');

    const chunks = Math.ceil(file.size / fileChunkSize);

    let success = true;

    for (let i = 0; i < chunks; i++) {
        const start = i * fileChunkSize;
        const end = Math.min(file.size, start + fileChunkSize);

        const chunk = file.slice(start, end);

        const formData = new FormData();

        formData.append('backup', new Blob([chunk], { type: file.type }), file.name);
        formData.append('currentChunk', (i + 1).toString());
        formData.append('totalChunks', chunks.toString());

        try {
            await $fetch('/api/users/@me/backups/upload', {
                method: 'POST',
                body: formData,
                retry: 3,
            });

            uploadProgress.value = Math.round((end / file.size) * 100);
        } catch (error: any) {
            success = false;
            $toast.error(error.data.message);
            break;
        }
    }

    uploading.value = false;
    uploadProgress.value = 0;

    if (success) {
        $toast.success('Backup uploaded successfully, you can now restore it');
    }
};

const { all, enter, leave } = animateCards();

onMounted(async () => {
    if (!backups.value.length) {
        const data = await $fetch('/api/users/@me/backups');

        backups.value = data.map((u) => ({
            ...u,
            createdAt: new Date(u.createdAt),
        }));

        isLoading.value = false;
        await nextTick();
    }

    all('backups', '.backupCard');
});

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
