import type themes from '~/styles/themes.json';

declare global {
    interface Window {
        get theme(): keyof typeof themes;
        set theme(theme: keyof typeof themes): void;
    }
}

export {};
