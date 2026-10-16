import type { UpdateStatus } from '../../shared/ipc';

export const useUpdateStatus = () => {
    return useState<UpdateStatus>('updateStatus', () => ({ state: 'idle' }));
};
