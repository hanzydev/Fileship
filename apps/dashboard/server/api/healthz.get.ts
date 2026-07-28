import pkg from '../../../../package.json';

export default defineEventHandler(() => {
    return {
        status: 'ok',
        version: pkg.version,
    };
});
