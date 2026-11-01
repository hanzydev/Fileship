import type { AppInfo } from '../../shared/ipc';

export const useAppInfo = () => useState<AppInfo>('appInfo');
