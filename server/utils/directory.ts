import { join } from 'pathe';

export const baseDirectory = join(
    process.env.NODE_ENV === 'development' ? './' : '../',
);

export const dataDirectory = join(
    baseDirectory,
    process.env.DATA_DIRECTORY || 'data',
);
