import { join, resolve } from 'pathe';

export const baseDirectory = join(import.meta.dev ? './' : process.cwd());

export const rootDirectory = join(baseDirectory, '..', '..');

export const dataDirectory = resolve(rootDirectory, useRuntimeConfig().dataDirectory || 'data');
