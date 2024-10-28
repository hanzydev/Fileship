<template>
    <UiModal v-model="isOpen" @closed="selectedMethod = bestMethod">
        <div
            flex="~ col items-center justify-center gap8"
            relative
            overflow-hidden
            p8
            text-center
            transition-height
            duration-250
            :style="{ height: `${height + 32}px` }"
        >
            <Transition
                enter-active-class="motion-safe:(animate-in fade-in data-[hubmode=true]:slide-in-from-left data-[hubmode=false]:slide-in-from-right animate-duration-250)"
                leave-active-class="motion-safe:(animate-out fade-out data-[hubmode=true]:slide-out-to-left data-[hubmode=false]:slide-out-to-right animate-duration-250)"
                :data-hubmode="hubMode"
                @enter="(el) => calculateHeight(el)"
                @after-enter="(el) => el.querySelector('input')?.focus()"
            >
                <div
                    v-if="hubMode"
                    flex="~ col items-center justify-center gap8"
                    absolute
                    left-auto
                    right-auto
                    wfull
                    p8
                >
                    <div flex="~ lt-sm:col items-center gap4">
                        <Icon name="heroicons-solid:hand" size="96" />
                        <div sm:text-left lt-sm:space-y-2>
                            <h3 sm:text-left>Multi-Factor Authentication</h3>
                            <p text-slate200>
                                This extra step shows that it's really you.
                            </p>
                        </div>
                    </div>
                    <div space-y-2 w="full sm:3/4">
                        <UiButton
                            v-if="passkeyAvailable"
                            alignment="right"
                            icon="heroicons-solid:chevron-right"
                            icon-size="20"
                            icon-class="mla"
                            wfull
                            flex-row-reverse
                            p="!y-3 !x-5"
                            @click="
                                selectedMethod = 'passkey';
                                hubMode = false;
                            "
                        >
                            Use a passkey
                        </UiButton>
                        <UiButton
                            v-if="totpAvailable"
                            alignment="right"
                            icon="heroicons-solid:chevron-right"
                            icon-size="20"
                            icon-class="mla"
                            wfull
                            flex-row-reverse
                            p="!y-3 !x-5"
                            @click="
                                selectedMethod = 'totp';
                                hubMode = false;
                            "
                        >
                            Use your authenticator app
                        </UiButton>
                        <UiButton
                            v-if="passwordAvailable"
                            alignment="right"
                            icon="heroicons-solid:chevron-right"
                            icon-size="20"
                            icon-class="mla"
                            wfull
                            flex-row-reverse
                            p="!y-3 !x-5"
                            @click="
                                selectedMethod = 'password';
                                hubMode = false;
                            "
                        >
                            Use your password
                        </UiButton>
                    </div>

                    <UiButton
                        alignment="center"
                        variant="accent"
                        icon="heroicons-solid:x"
                        icon-size="24"
                        wfull
                        gap2
                        :disabled
                        @click="isOpen = false"
                    >
                        Cancel
                    </UiButton>
                </div>
                <div
                    v-else
                    flex="~ col items-center justify-center gap8"
                    data-selectedmethod
                    absolute
                    left-auto
                    right-auto
                    wfull
                    p8
                >
                    <div flex="~ lt-sm:col items-center gap4">
                        <Icon name="heroicons-solid:hand" size="96" />
                        <div sm:text-left lt-sm:space-y-2>
                            <h3 sm:text-left>Multi-Factor Authentication</h3>
                            <p text-slate200>
                                {{
                                    selectedMethod === 'passkey'
                                        ? 'Verify your identity with a passkey.'
                                        : selectedMethod === 'totp'
                                          ? 'Verify your identity with your authenticator app.'
                                          : 'Verify your identity with your current password.'
                                }}
                            </p>
                        </div>
                    </div>

                    <div v-if="selectedMethod === 'passkey'">
                        <UiButton
                            variant="accent"
                            icon="heroicons-solid:finger-print"
                            icon-size="20"
                            gap2
                            @click="handlePasskey"
                        >
                            Authenticate with passkey
                        </UiButton>
                    </div>
                    <div v-else-if="selectedMethod === 'totp'">
                        <UiTotpInput
                            v-model="totp"
                            required
                            :error
                            :disabled
                            @got="
                                (totp) =>
                                    emit('got', { type: 'totp', data: totp })
                            "
                        />
                    </div>
                    <form
                        v-else-if="selectedMethod === 'password'"
                        wfull
                        @submit.prevent="
                            emit('got', { type: 'password', data: password })
                        "
                    >
                        <UiInput
                            v-model="password"
                            type="password"
                            label="Password"
                            required
                            wfull
                            :error
                            :disabled
                        />

                        <button type="submit" hidden></button>
                    </form>

                    <div
                        flex="~ gap2 md:(justify-between items-center) lt-md:(col gap4)"
                        wfull
                    >
                        <p
                            v-if="methods.length > 1"
                            cursor-pointer
                            text-slate200
                            font-medium
                            hover:underline
                            @click="hubMode = true"
                        >
                            Verify with something else
                        </p>

                        <div flex="~ gap2" mla lt-md:wfull>
                            <UiButton
                                alignment="center"
                                icon="heroicons-solid:x"
                                icon-size="24"
                                wfull
                                gap2
                                md:w10rem
                                :disabled
                                @click="
                                    isOpen = false;
                                    emit('cancel');
                                "
                            >
                                Cancel
                            </UiButton>
                            <UiButton
                                v-if="selectedMethod !== 'passkey'"
                                alignment="center"
                                variant="accent"
                                icon="heroicons-solid:check"
                                icon-size="24"
                                wfull
                                gap2
                                md:w10rem
                                :loading="disabled"
                                :disabled
                                @click="
                                    emit('got', {
                                        type: selectedMethod,
                                        data:
                                            selectedMethod === 'totp'
                                                ? totp
                                                : password,
                                    })
                                "
                            >
                                Verify
                            </UiButton>
                        </div>
                    </div>
                </div>
            </Transition>
        </div>
    </UiModal>
</template>

<script setup lang="ts">
import { toast } from 'vue-sonner';

import { startAuthentication } from '@simplewebauthn/browser';
import type {
    AuthenticationResponseJSON,
    PublicKeyCredentialRequestOptionsJSON,
} from '@simplewebauthn/types';

const isOpen = defineModel<boolean>({ required: false, default: true });

type MethodType = 'passkey' | 'totp' | 'password';

const { methods } = defineProps<{
    error?: string;
    disabled?: boolean;
    methods: {
        type: MethodType;
        challange?: PublicKeyCredentialRequestOptionsJSON;
    }[];
}>();

const emit = defineEmits<{
    got: [
        {
            type: MethodType;
            data:
                | string
                | {
                      expectedChallenge: string;
                      authenticationResponse: AuthenticationResponseJSON;
                  };
        },
    ];
    cancel: [];
}>();

const passkeyAvailable = computed(() =>
    methods.some((m) => m.type === 'passkey'),
);
const totpAvailable = computed(() => methods.some((m) => m.type === 'totp'));

const passwordAvailable = computed(() =>
    methods.some((m) => m.type === 'password'),
);

const bestMethod = computed(() =>
    passkeyAvailable.value
        ? 'passkey'
        : totpAvailable.value
          ? 'totp'
          : 'password',
);

const selectedMethod = ref<MethodType>(bestMethod.value);

const password = ref('');
const totp = ref('');

const hubMode = ref(false);
const height = ref(320 /** initial */);

const calculateHeight = (el: Element) => {
    height.value = el.clientHeight;
};

const handlePasskey = async () => {
    const optionsJSON = methods.find((m) => m.type === 'passkey')!.challange!;

    try {
        const authenticationResponse = await startAuthentication({
            optionsJSON,
        });

        emit('got', {
            type: 'passkey',
            data: {
                expectedChallenge: optionsJSON.challenge,
                authenticationResponse,
            },
        });
    } catch {
        toast.error('Failed to verify passkey');
    }
};

watch(bestMethod, () => (selectedMethod.value = bestMethod.value));

watch(isOpen, (value) => {
    if (value) {
        nextTick(() =>
            calculateHeight(document.querySelector('[data-selectedmethod]')!),
        );
    }
});
</script>
