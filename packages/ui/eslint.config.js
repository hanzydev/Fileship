import nuxt from '@fileship/eslint-config/nuxt';

import withNuxt from './.nuxt/eslint.config.mjs';

export default withNuxt(nuxt, { ignores: ['app/components/ui/**'] });
