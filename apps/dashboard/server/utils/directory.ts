import { join } from 'pathe';

export const baseDirectory = join(import.meta.dev ? './' : process.cwd());

export const rootDirectory = join(baseDirectory, '..', '..');

export const dataDirectory = join(rootDirectory, useRuntimeConfig().dataDirectory || 'data');
