<template>
    <div>
        <Head>
            <Title>My Account</Title>
        </Head>

        <ModalsVerifyTotp
            v-if="currentUser!.totpEnabled"
            v-model="verifyModalOpen"
            :error="verificationError"
            :disabled="userUpdating"
            @got="handleUserEdit"
        />
        <ModalsVerifyUserPassword
            v-else
            v-model="verifyModalOpen"
            :error="verificationError"
            :disabled="userUpdating"
            @got="handleUserEdit"
        />

        <ModalsVerifyTotp
            v-if="currentUser!.totpEnabled"
            v-model="disable2FaModalOpen"
            :error="change2FaError"
            :disabled="twoFaUpdating"
            @got="(totp) => change2Fa(false, totp)"
            @closed="
                twoFaEnabled = currentUser!.totpEnabled;
                change2FaError = undefined;
            "
        />
        <ModalsVerifyUserPassword
            v-else
            v-model="verify2FaModalOpen"
            :error="verify2FaError"
            :disabled="twoFaUpdating"
            @got="generate2FaQrCode"
            @closed="verify2FaError = undefined"
            @outer-click="twoFaEnabled = currentUser!.totpEnabled"
            @cancel="twoFaEnabled = currentUser!.totpEnabled"
        />

        <UiModal
            v-model="enable2FaModalOpen"
            flex="~ col items-center justify-center gap8"
            p8
            text-center
            @closed="
                twoFaEnabled = currentUser!.totpEnabled;
                twoFaQrCode = '';
                change2FaError = undefined;
            "
        >
            <h3>Enable Two-Factor Authentication</h3>

            <p text-slate200 font-medium="!">
                Scan the QR code below with your authenticator app to enable
                two-factor authentication.
            </p>

            <div relative p8>
                <div
                    absolute
                    left-0
                    top-0
                    h20
                    w20
                    b="l-5 t-5 fs-accent"
                    rounded="tl-md"
                    class="zortr"
                ></div>
                <div
                    absolute
                    right-0
                    top-0
                    h20
                    w20
                    b="r-5 t-5 fs-accent"
                    rounded="tr-md"
                    class="zortr"
                ></div>
                <img :src="twoFaQrCode" h48 w48 rounded draggable="false" />
                <div
                    absolute
                    bottom-0
                    left-0
                    h20
                    w20
                    b="l-5 b-5 fs-accent"
                    rounded="bl-md"
                    class="zortr"
                ></div>
                <div
                    absolute
                    bottom-0
                    right-0
                    h20
                    w20
                    b="r-5 b-5 fs-accent"
                    rounded="br-md"
                    class="zortr"
                ></div>
            </div>

            <div wfit>
                <UiTotpInput
                    flex="~ col items-center justify-center text-center"
                    :error="change2FaError"
                    :disabled="twoFaUpdating"
                    @got="(totp) => change2Fa(true, totp)"
                />
            </div>

            <UiButton
                alignment="center"
                variant="accent"
                icon="heroicons-solid:x"
                icon-size="24"
                wfull
                gap2
                @click="enable2FaModalOpen = false"
            >
                Cancel
            </UiButton>
        </UiModal>

        <UiModal v-model="shareXConfigModal.open" p8 space-y-4>
            <h2>Settings</h2>

            <div space-y-1>
                <UiLabel :for="shareXConfigTypeId">Config Type</UiLabel>

                <UiTabs
                    :id="shareXConfigTypeId"
                    v-model="shareXConfigModal.settings.configType"
                    :items="[
                        {
                            label: 'File Uploader',
                            icon: 'heroicons-solid:document',
                        },
                        {
                            label: 'URL Shortener',
                            icon: 'heroicons:link-16-solid',
                        },
                    ]"
                    width-full
                />
            </div>

            <div
                v-if="shareXConfigModal.settings.configType === 'File Uploader'"
                space-y-1
            >
                <UiLabel :for="shareXFileNameTypeId">File Name Type</UiLabel>

                <UiTabs
                    :id="shareXFileNameTypeId"
                    v-model="shareXConfigModal.settings.fileNameType"
                    :items="[
                        {
                            label: 'Random',
                            icon: 'heroicons-solid:cube',
                        },
                        {
                            label: 'UUID',
                            icon: 'heroicons-solid:key',
                        },
                        {
                            label: 'Original',
                            icon: 'heroicons-solid:document',
                        },
                    ]"
                    width-full
                />
            </div>
            <UiInput
                v-model="shareXConfigModal.settings.password!"
                wfull
                label="Password"
                type="password"
            />
            <UiInput
                v-model="shareXConfigModal.settings.maxViews"
                wfull
                label="Max Views"
                caption="Set to 0 for unlimited views"
                type="number"
                :min="0"
            />
            <ExpirationPicker v-model="shareXConfigModal.settings.expiration">
                <UiInput
                    v-model="shareXConfigModal.settings.expiration.label"
                    label="Expiration"
                    type="string"
                    readonly
                    wfull
                    cursor-pointer="!"
                />
            </ExpirationPicker>
            <CompressionPicker
                v-if="shareXConfigModal.settings.configType === 'File Uploader'"
                v-model="shareXConfigModal.settings.compression"
            >
                <UiInput
                    v-model="shareXConfigModal.settings.compression.label"
                    label="Compression"
                    type="string"
                    readonly
                    wfull
                    cursor-pointer="!"
                />
            </CompressionPicker>

            <div grid="~ cols-2 gap4">
                <UiButton
                    alignment="center"
                    icon="heroicons-solid:x"
                    icon-size="24"
                    wfull
                    gap2
                    @click="shareXConfigModal.open = false"
                >
                    Cancel
                </UiButton>
                <UiButton
                    wfull
                    gap2
                    alignment="center"
                    variant="accent"
                    type="submit"
                    icon="heroicons-solid:download"
                    icon-size="20"
                    @click="generateShareXConfig"
                >
                    Generate
                </UiButton>
            </div>
        </UiModal>

        <div>
            <h2>Account</h2>

            <div ref="content" mt6 space-y-6>
                <UiExpander op0>
                    <div flex="~ gap2 items-center">
                        <Icon name="heroicons-solid:identification" size="24" />
                        <h5>My Account</h5>
                    </div>
                    <template #content>
                        <form space-y-4 @submit.prevent="handleUserEdit()">
                            <p text-slate200>
                                Update your account information here.
                            </p>

                            <div grid="~ sm:cols-2 gap4">
                                <UiInput
                                    v-model="
                                        userEditData.cloned.value.username!
                                    "
                                    label="Username"
                                    rounded="!"
                                    required
                                    wfull
                                    :error="
                                        userFormErrors?.username?._errors?.[0]
                                    "
                                    :disabled="userUpdating"
                                />
                                <UiInput
                                    v-model="
                                        userEditData.cloned.value.password!
                                    "
                                    label="Password"
                                    wfull
                                    rounded="!"
                                    type="password"
                                    caption="If you leave the password field empty, your
                                password will not be updated."
                                    :error="
                                        userFormErrors?.password?._errors?.[0]
                                    "
                                    :disabled="userUpdating"
                                />
                            </div>

                            <UiButton
                                wfull
                                rounded="!"
                                gap2
                                alignment="center"
                                variant="accent"
                                type="submit"
                                icon="heroicons:pencil-16-solid"
                                icon-size="20"
                                :loading="userUpdating"
                                :disabled="userUpdating"
                            >
                                Save
                            </UiButton>
                        </form>
                    </template>
                </UiExpander>

                <UiExpander op0>
                    <div flex="~ gap2 items-center">
                        <Icon name="heroicons-solid:photograph" size="24" />
                        <h5>Avatar</h5>
                    </div>
                    <template #content>
                        <form space-y-4 @submit.prevent="handleAvatarEdit">
                            <p text-slate200>Update your avatar here.</p>

                            <div grid="~ gap4 sm:cols-3">
                                <div
                                    relative
                                    rounded
                                    border="1 dashed fs-overlay-4"
                                    active:scale-95
                                    motion-safe:transition-all
                                    :class="
                                        !(avatarUpdating || avatarResetting) &&
                                        'hover:(border-fs-accent border-solid)'
                                    "
                                >
                                    <input
                                        :key="
                                            userEditData.cloned.value.avatar
                                                ?.name
                                        "
                                        absolute
                                        z10
                                        hfull
                                        wfull
                                        op0
                                        :class="
                                            avatarUpdating || avatarResetting
                                                ? 'cursor-not-allowed'
                                                : 'cursor-pointer'
                                        "
                                        type="file"
                                        accept="image/*"
                                        :disabled="
                                            avatarUpdating || avatarResetting
                                        "
                                        @change.stop.prevent="
                                            (event) =>
                                                (userEditData.cloned.value.avatar =
                                                    (
                                                        event.target as HTMLInputElement
                                                    ).files![0]!)
                                        "
                                    />
                                    <UiInput
                                        :model-value="
                                            userEditData.cloned.value.avatar
                                                ?.name!
                                        "
                                        rounded="!"
                                        readonly
                                        wfull
                                        placeholder="Choose a file"
                                        :disabled="
                                            avatarUpdating || avatarResetting
                                        "
                                        ring-none
                                    />
                                </div>
                                <UiButton
                                    wfull
                                    rounded="!"
                                    gap2
                                    alignment="center"
                                    variant="dangerFill"
                                    type="submit"
                                    icon="heroicons-solid:trash"
                                    icon-size="20"
                                    :loading="avatarResetting"
                                    :disabled="
                                        avatarUpdating ||
                                        avatarResetting ||
                                        !currentUser!.avatar
                                    "
                                    @click="
                                        userEditData.cloned.value.avatar = null
                                    "
                                >
                                    Reset
                                </UiButton>
                                <UiButton
                                    wfull
                                    rounded="!"
                                    gap2
                                    alignment="center"
                                    variant="accent"
                                    type="submit"
                                    icon="heroicons:pencil-16-solid"
                                    icon-size="20"
                                    :loading="avatarUpdating"
                                    :disabled="
                                        avatarUpdating ||
                                        avatarResetting ||
                                        !userEditData.cloned.value.avatar
                                    "
                                >
                                    Save
                                </UiButton>
                            </div>
                        </form>
                    </template>
                </UiExpander>

                <UiExpander op0>
                    <div flex="~ gap2 items-center">
                        <Icon name="heroicons-solid:globe-alt" size="24" />
                        <h5>Domains</h5>
                    </div>

                    <template #content>
                        <form space-y-4 @submit.prevent="handleDomainsEdit">
                            <p text-slate200>
                                Configure your domains. These domains will be
                                used to output a random domain during upload.
                            </p>

                            <UiInput
                                label="Domains"
                                rounded="!"
                                wfull
                                caption="Separate multiple domains with a comma. Example: domain.com, i.domain2.com."
                                :disabled="domainsUpdating"
                                :model-value="
                                    domainsEditData.cloned.value.join(', ')
                                "
                                @update:model-value="
                                    (value) =>
                                        (domainsEditData.cloned.value = (
                                            value as string
                                        )
                                            .split(',')
                                            .map((domain) => domain.trim()))
                                "
                            />

                            <UiButton
                                wfull
                                rounded="!"
                                gap2
                                alignment="center"
                                variant="accent"
                                type="submit"
                                icon="heroicons:pencil-16-solid"
                                icon-size="20"
                                :loading="domainsUpdating"
                                :disabled="domainsUpdating"
                            >
                                Save
                            </UiButton>
                        </form>
                    </template>
                </UiExpander>

                <UiExpander op0>
                    <div flex="~ gap2 items-center">
                        <Icon name="heroicons-solid:shield-check" size="24" />
                        <h5>Two-Factor Authentication</h5>
                    </div>

                    <template #content>
                        <div space-y-4>
                            <p text-slate200>
                                Configuring an authenticator app is a good way
                                to add an extra layer of security to your
                                {{ appConfig.site.name }} account to make sure
                                that only you have the ability to log in.
                            </p>

                            <div flex="~ gap2 items-center">
                                <UiSwitch
                                    v-model="twoFaEnabled"
                                    :disabled="twoFaUpdating"
                                />
                                <span font-medium="!">
                                    Enable Two-Factor Authentication
                                </span>
                            </div>
                        </div>
                    </template>
                </UiExpander>

                <UiExpander op0>
                    <div flex="~ gap2 items-center">
                        <Icon name="heroicons-solid:lock-closed" size="24" />
                        <h5>Embed Configuration</h5>
                    </div>
                    <template #content>
                        <form space-y-4 @submit.prevent="handleEmbedEdit">
                            <p text-slate200>
                                Configure how your files are embedded when
                                shared.
                            </p>

                            <div grid="~ gap4 sm:cols-2">
                                <UiInput
                                    v-model="embedEditData.cloned.value.title!"
                                    label="Title"
                                    wfull
                                    rounded="!"
                                    :disabled="embedUpdating"
                                />
                                <UiInput
                                    v-model="
                                        embedEditData.cloned.value.description!
                                    "
                                    label="Description"
                                    wfull
                                    rounded="!"
                                    :disabled="embedUpdating"
                                />
                                <UiInput
                                    v-model="
                                        embedEditData.cloned.value.siteName!
                                    "
                                    label="Site Name"
                                    wfull
                                    rounded="!"
                                    :disabled="embedUpdating"
                                />
                                <ColorPicker
                                    v-model="embedEditData.cloned.value.color!"
                                >
                                    <div relative>
                                        <div
                                            absolute
                                            left-3.5
                                            top-10
                                            z10
                                            h5
                                            w5
                                            rounded-full
                                            :style="{
                                                backgroundColor:
                                                    embedEditData.cloned.value
                                                        .color,
                                            }"
                                        ></div>
                                        <UiInput
                                            v-model="
                                                embedEditData.cloned.value
                                                    .color!
                                            "
                                            label="Color"
                                            wfull
                                            pl11
                                            ring-1
                                            rounded="!"
                                            transition-none="!"
                                            :error="
                                                userFormErrors?.embed?.color
                                                    ?._errors?.[0]
                                            "
                                            :style="{
                                                '--un-ring-color':
                                                    embedEditData.cloned.value
                                                        .color,
                                            }"
                                            :min="1"
                                            :max="7"
                                            :disabled="embedUpdating"
                                        />
                                    </div>
                                </ColorPicker>
                            </div>
                            <h6>
                                Available parameters:
                                <span font-medium="!">
                                    {fileName}, {mimeType}, {size}, {createdAt}
                                    and {now}
                                </span>
                            </h6>
                            <div flex="~ gap2 items-center">
                                <UiSwitch
                                    v-model="embedEditData.cloned.value.enabled"
                                    :disabled="embedUpdating"
                                />
                                <span font-medium="!">Enable embeds</span>
                            </div>

                            <UiButton
                                wfull
                                gap2
                                alignment="center"
                                variant="accent"
                                type="submit"
                                icon="heroicons:pencil-16-solid"
                                icon-size="20"
                                :loading="userUpdating"
                                :disabled="embedUpdating"
                            >
                                Save
                            </UiButton>
                        </form>
                    </template>
                </UiExpander>

                <div op0>
                    <UiButton
                        gap2
                        variant="accent"
                        icon="heroicons-solid:download"
                        icon-size="20"
                        @click="shareXConfigModal.open = true"
                    >
                        Generate ShareX Config
                    </UiButton>
                </div>
            </div>
        </div>
    </div>
</template>

<script setup lang="ts">
import { Cubic, gsap } from 'gsap';
import { render } from 'vue';
import { toast } from 'vue-sonner';

const embed = useEmbed();
const domains = useDomains();
const appConfig = useAppConfig();
const currentUser = useAuthUser();

const contentRef = useTemplateRef<HTMLDivElement>('content');

const shareXConfigModal = reactive<{
    open: boolean;
    settings: {
        configType: 'File Uploader' | 'URL Shortener';
        fileNameType: 'Random' | 'UUID' | 'Original';
        maxViews: number;
        password: string | null;
        expiration: {
            label: string;
            value: number | null;
        };
        compression: {
            label: string;
            value: number;
        };
    };
}>({
    open: false,
    settings: {
        configType: 'File Uploader',
        fileNameType: 'Random',
        maxViews: 0,
        password: null,
        expiration: {
            label: 'Never',
            value: null,
        },
        compression: {
            label: 'None',
            value: 0,
        },
    },
});

const shareXConfigTypeId = useId();
const shareXFileNameTypeId = useId();

const userEditData = useCloned({
    ...currentUser.value,
    password: '',
    avatar: undefined as File | undefined | null,
});

const embedEditData = useCloned(embed);
const domainsEditData = useCloned(domains);

const userUpdating = ref(false);
const userFormErrors = ref();

const verifyModalOpen = ref(false);
const verificationError = ref<string>();

const change2FaError = ref<string>();

const twoFaQrCode = ref<string>();
const twoFaEnabled = ref(currentUser.value!.totpEnabled);
const twoFaUpdating = ref(false);

const enable2FaModalOpen = ref(false);
const disable2FaModalOpen = ref(false);

const verify2FaError = ref<string>();
const verify2FaModalOpen = ref(false);

const domainsUpdating = ref(false);
const embedUpdating = ref(false);

const avatarUpdating = ref(false);
const avatarResetting = ref(false);

const handleAvatarEdit = async () => {
    const avatar = userEditData.cloned.value.avatar;
    const isResetting = [null, undefined].includes(avatar as never);

    if (isResetting) avatarResetting.value = true;
    else avatarUpdating.value = true;

    try {
        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: {
                avatar: isResetting
                    ? null
                    : Buffer.from(
                          await new Promise<ArrayBuffer>((resolve) => {
                              const reader = new FileReader();
                              reader.readAsArrayBuffer(avatar!);
                              reader.onload = () =>
                                  resolve(reader.result as never);
                          }),
                      ).toString('base64'),
            },
        });

        userEditData.cloned.value.avatar = undefined;

        toast.success('Avatar updated successfully');
    } catch (error: any) {
        toast.error(error.data.message);
    }

    avatarUpdating.value = false;
    avatarResetting.value = false;
};

const handleUserEdit = async (verificationData?: string) => {
    userUpdating.value = true;
    userFormErrors.value = {};
    verificationError.value = undefined;

    try {
        await $fetch(`/api/users/${currentUser.value!.id}`, {
            method: 'PATCH',
            body: {
                username: userEditData.cloned.value.username,
                password: userEditData.cloned.value.password || undefined,
                verificationData,
            },
        });

        verifyModalOpen.value = false;
        userEditData.cloned.value.password = '';
        userEditData.cloned.value.avatar = undefined;

        toast.success('Account updated successfully');
    } catch (error: any) {
        userFormErrors.value = error.data.data;

        if (!error.data.data && error.data.message) {
            if (verifyModalOpen.value) {
                verificationError.value = error.data.message;
            } else {
                verifyModalOpen.value = true;
            }
        }
    }

    userUpdating.value = false;
};

const handleEmbedEdit = async () => {
    embedUpdating.value = true;

    await $fetch('/api/users/@me/embed', {
        method: 'PATCH',
        body: embedEditData.cloned.value,
    });

    embedUpdating.value = false;

    toast.success('Embed config updated successfully');
};

const handleDomainsEdit = async () => {
    domainsUpdating.value = true;

    await $fetch('/api/users/@me/domains', {
        method: 'PUT',
        body: domainsEditData.cloned.value,
    });

    domainsUpdating.value = false;

    toast.success('Domains updated successfully');
};

const change2Fa = async (enabled: boolean, totp: string) => {
    twoFaUpdating.value = true;
    change2FaError.value = undefined;

    try {
        await $fetch('/api/auth/totp', {
            method: 'PUT',
            body: {
                enabled,
                totp,
            },
        });

        toast.success(
            `Two-Factor Authentication ${
                enabled ? 'enabled' : 'disabled'
            } successfully`,
        );

        enable2FaModalOpen.value = false;
        disable2FaModalOpen.value = false;
    } catch (error: any) {
        change2FaError.value = error.data.message;
    }

    twoFaUpdating.value = false;
};

const generate2FaQrCode = async (verificationData?: string) => {
    twoFaUpdating.value = true;
    verify2FaError.value = undefined;

    try {
        const { base64 } = await $fetch('/api/auth/totp/qrcode', {
            method: 'POST',
            body: { verificationData },
        });

        twoFaQrCode.value = base64;
        verify2FaModalOpen.value = false;
        enable2FaModalOpen.value = true;
    } catch (error: any) {
        if (verify2FaModalOpen.value) verify2FaError.value = error.data.message;
        else verify2FaModalOpen.value = true;
    }

    twoFaUpdating.value = false;
};

const handle2FaChange = (enabled: boolean) => {
    if (enabled === currentUser.value!.totpEnabled) return;

    if (enabled) generate2FaQrCode();
    else disable2FaModalOpen.value = true;
};

const generateShareXFileUploaderConfig = () => {
    const config = {
        Version: '16.1.0',
        Name: `${appConfig.site.name} - File Uploader`,
        DestinationType: 'ImageUploader, TextUploader, FileUploader',
        RequestMethod: 'POST',
        RequestURL: `${useRequestURL().origin}/api/files`,
        Headers: {
            Authorization: useCookie('sessionId').value,
        },
        URL: '{json:url}',
        ErrorMessage: '{json:error}',
        Body: 'MultipartFormData',
        FileFormName: 'file',
        Arguments: {
            fileNameType: shareXConfigModal.settings.fileNameType,
            password: shareXConfigModal.settings.password,
            expiration: shareXConfigModal.settings.expiration.value,
            maxViews: shareXConfigModal.settings.maxViews,
            compression: shareXConfigModal.settings.compression.value,
        },
    };

    const vnode = h('a', {
        href: URL.createObjectURL(
            new Blob([JSON.stringify(config)], {
                type: 'application/json',
            }),
        ),
        download: 'FileUploader.sxcu',
    });

    render(vnode, document.body);
    nextTick(() => {
        vnode.el!.click();
        vnode.el!.remove();
    });
};

const generateShareXUrlShortenerConfig = () => {
    const config = {
        Version: '16.1.0',
        Name: `${appConfig.site.name} - URL Shortener`,
        DestinationType: 'URLShortener',
        RequestMethod: 'POST',
        RequestURL: `${useRequestURL().origin}/api/urls`,
        Headers: {
            Authorization: useCookie('sessionId').value,
        },
        URL: '{json:url}',
        ErrorMessage: '{json:error}',
        Body: 'JSON',
        Data: JSON.stringify({
            maxViews: shareXConfigModal.settings.maxViews,
            password: shareXConfigModal.settings.password,
            expiration: shareXConfigModal.settings.expiration.value,
            destinationUrl: '{input}',
        }),
    };

    const vnode = h('a', {
        href: URL.createObjectURL(
            new Blob([JSON.stringify(config)], {
                type: 'application/json',
            }),
        ),
        download: 'UrlShortener.sxcu',
    });

    render(vnode, document.body);
    nextTick(() => {
        vnode.el!.click();
        vnode.el!.remove();
    });
};

const generateShareXConfig = () => {
    if (shareXConfigModal.settings.configType === 'File Uploader') {
        generateShareXFileUploaderConfig();
    } else {
        generateShareXUrlShortenerConfig();
    }
};

onMounted(() => {
    gsap.set(contentRef.value!.children, { opacity: 1 });
    gsap.from(contentRef.value!.children, {
        opacity: 0,
        y: 10,
        duration: 0.3,
        stagger: 0.1,
        filter: 'blur(0.125rem)',
        ease: Cubic.easeOut,
    });
});

watch(
    embedEditData.cloned,
    (value) => {
        if (value.color[0] !== '#') value.color = `#${value.color}`;
    },
    { deep: true },
);

watch(
    () => currentUser.value!.totpEnabled,
    (value) => (twoFaEnabled.value = value),
);

watch(twoFaEnabled, handle2FaChange);

definePageMeta({
    layout: 'dashboard',
    middleware: 'user-only',
});
</script>
