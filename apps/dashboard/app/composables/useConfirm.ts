export type ConfirmOptions = {
    title: string;
    description?: string;
    confirmText?: string;
    destructive?: boolean;
};

const confirmState = ref<(ConfirmOptions & { resolve: (value: boolean) => void }) | null>(null);

export const useConfirm = () => {
    return confirmState;
};

export const askConfirmation = (options: ConfirmOptions) => {
    return new Promise<boolean>((resolve) => {
        confirmState.value = { ...options, resolve };
    });
};
