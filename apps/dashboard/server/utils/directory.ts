import { join } from 'pathe';

export const baseDirectory = join(process.env.NODE_ENV === 'development' ? './' : '../');

export const rootDirectory = join(baseDirectory, '..', '..');

export const dataDirectory = join(rootDirectory, process.env.DATA_DIRECTORY || 'data');
