import { defaultEmbed } from '~~/utils/constants';
import type { IEmbed } from '~~/utils/types';

export const useEmbed = () => {
    return useState<IEmbed>('embed', () => defaultEmbed);
};
