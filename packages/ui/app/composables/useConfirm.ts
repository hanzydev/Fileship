export type ConfirmOptions = {
    title: string;
    description?: string;
    confirmText?: string;
    destructive?: boolean;
    option?: { label: string; value: boolean };
    action?: () => Promise<unknown>;
    secondary?: { text: string; destructive?: boolean; action: () => Promise<unknown> };
};

const confirmState = ref<(ConfirmOptions & { resolve: (value: boolean) => void }) | null>(null);

export const useConfirm = () => {
    return confirmState;
};

export const askConfirmation = (options: ConfirmOptions) => {
    confirmState.value?.resolve(false);

    return new Promise<boolean>((resolve) => {
        confirmState.value = { ...options, resolve };
    });
};
