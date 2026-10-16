import type { Account } from '../../shared/ipc';

export const useAccount = () => {
    return useState<Account | null>('account', () => null);
};
