import { platform } from './native.mjs';

await import(`./${platform}/${process.argv[2]}.mjs`);
