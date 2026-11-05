import type { DesktopDownload, HarborItemKind } from '~/types/desktop';

export const useDesktopApp = () => {
    const isDesktop = useState('desktopApp', () => false);
    const downloads = useState<DesktopDownload[]>('desktopDownloads', () => []);
    const downloadsStarted = useState('desktopDownloadsStarted', () => 0);
    const isHarborAvailable = useState('desktopHarbor', () => false);
    const currentUser = useAuthUser();
    const { $toast, $i18n } = useNuxtApp();

    const bridge = () => window.fileshipDesktop!;

    const isInHarbor = (item: { authorId: string; deletedAt?: Date | string | null }) =>
        isHarborAvailable.value && !item.deletedAt && item.authorId === currentUser.value?.id;

    const revealInHarbor = async (kind: HarborItemKind, id: string) => {
        const isFound = await bridge()
            .harbor.reveal(kind, id)
            .catch(() => false);

        if (!isFound) $toast.error($i18n.t('actions.harbor.notFound'));
    };

    return {
        isDesktop,
        downloads,
        downloadsStarted,
        isHarborAvailable,
        bridge,
        isInHarbor,
        revealInHarbor,
    };
};
