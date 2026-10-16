import simpleImportSort from 'eslint-plugin-simple-import-sort';

export default [
    {
        rules: {
            '@typescript-eslint/no-explicit-any': 'off',

            'vue/multi-word-component-names': 'off',
            'vue/max-attributes-per-line': 'off',
            'vue/require-default-prop': 'off',
            'vue/html-self-closing': 'off',
            'vue/html-closing-bracket-newline': 'off',
            'vue/first-attribute-linebreak': 'off',
            'vue/no-v-html': 'off',
            'vue/require-valid-default-prop': 'off',
            'vue/no-multiple-template-root': 'off',

            'no-shadow-restricted-names': 'off',
        },
    },
    {
        plugins: {
            'simple-import-sort': simpleImportSort,
        },
        rules: {
            'simple-import-sort/imports': [
                'error',
                {
                    groups: [
                        // Side effect imports.
                        ['^\\u0000'],

                        // Node.js builtins prefixed with `node:`.
                        ['^node:'],

                        // Packages.
                        ['^\\w'],

                        // Packages prefixed with `@`.
                        ['^@\\w'],

                        // Nuxt aliases.
                        ['^#', '^~~/'],

                        // Relative imports.
                        ['^\\.'],
                    ],
                },
            ],
            'simple-import-sort/exports': 'error',
        },
    },
];
