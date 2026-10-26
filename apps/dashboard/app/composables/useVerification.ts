import type { PublicKeyCredentialRequestOptionsJSON } from '@simplewebauthn/browser';

export type VerificationMethod =
    | { type: 'password' | 'totp' | 'oidc' }
    | {
          type: 'passkey';
          ticket: string;
          authenticationOptions: PublicKeyCredentialRequestOptionsJSON;
      };

const verificationState = ref<{
    methods: VerificationMethod[];
    error?: string;
    isSubmitting: boolean;
    submit: (verificationData: unknown) => Promise<void>;
    cancel: () => void;
} | null>(null);

export const useVerification = () => {
    return verificationState;
};

export const withVerification = async <T>(action: (verificationData?: unknown) => Promise<T>) => {
    try {
        return ((await action()) ?? true) as NonNullable<T>;
    } catch (error: any) {
        const methods = error.data?.data?.mfa?.methods;
        if (!methods) throw error;

        verificationState.value?.cancel();

        const { $i18n } = useNuxtApp();

        return new Promise<NonNullable<T> | null>((resolve, reject) => {
            verificationState.value = {
                methods,
                isSubmitting: false,
                submit: async (verificationData) => {
                    verificationState.value!.isSubmitting = true;
                    verificationState.value!.error = undefined;

                    try {
                        const result = await action(verificationData);
                        verificationState.value = null;
                        resolve((result ?? true) as NonNullable<T>);
                    } catch (error: any) {
                        if (error.status !== 401 && !error.data?.data?.mfa) {
                            verificationState.value = null;
                            reject(error);
                            return;
                        }

                        verificationState.value!.error =
                            error.data?.message || $i18n.t('actions.verificationFailed');
                        verificationState.value!.isSubmitting = false;
                    }
                },
                cancel: () => {
                    verificationState.value = null;
                    resolve(null);
                },
            };
        });
    }
};
