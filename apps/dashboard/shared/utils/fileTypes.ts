export const TEXT_FILE_TYPES = [
    {
        label: 'Plain Text',
        mime: 'text/plain',
        extension: 'txt',
    },
    {
        label: 'Markdown',
        mime: 'text/markdown',
        extension: 'md',
    },
    {
        label: 'HTML',
        mime: 'text/html',
        extension: 'html',
    },
    {
        label: 'CSS',
        mime: 'text/css',
        extension: 'css',
    },
    {
        label: 'SCSS',
        mime: 'text/x-scss',
        extension: 'scss',
    },
    {
        label: 'LESS',
        mime: 'text/x-less',
        extension: 'less',
    },
    {
        label: 'JavaScript',
        mime: 'application/javascript',
        extension: 'js',
    },
    {
        label: 'TypeScript',
        mime: 'application/typescript',
        extension: 'ts',
    },
    {
        label: 'JSON',
        mime: 'application/json',
        extension: 'json',
    },
    {
        label: 'YAML',
        mime: 'text/yaml',
        extension: 'yml',
    },
    {
        label: 'Python',
        mime: 'text/x-python',
        extension: 'py',
    },
    {
        label: 'Java',
        mime: 'text/x-java',
        extension: 'java',
    },
    {
        label: 'C',
        mime: 'text/x-csrc',
        extension: 'c',
    },
    {
        label: 'C++',
        mime: 'text/x-c++src',
        extension: 'cpp',
    },
    {
        label: 'C#',
        mime: 'text/x-csharp',
        extension: 'cs',
    },
    {
        label: 'Go',
        mime: 'text/x-go',
        extension: 'go',
    },
    {
        label: 'Ruby',
        mime: 'text/x-ruby',
        extension: 'rb',
    },
    {
        label: 'PHP',
        mime: 'application/x-php',
        extension: 'php',
    },
    {
        label: 'Swift',
        mime: 'text/x-swift',
        extension: 'swift',
    },
    {
        label: 'Kotlin',
        mime: 'text/x-kotlin',
        extension: 'kt',
    },
    {
        label: 'Rust',
        mime: 'text/x-rustsrc',
        extension: 'rs',
    },
    {
        label: 'Perl',
        mime: 'text/x-perl',
        extension: 'pl',
    },
    {
        label: 'Lua',
        mime: 'text/x-lua',
        extension: 'lua',
    },
    {
        label: 'Bash',
        mime: 'application/x-sh',
        extension: 'sh',
    },
    {
        label: 'R',
        mime: 'text/x-r',
        extension: 'r',
    },
    {
        label: 'XML',
        mime: 'application/xml',
        extension: 'xml',
    },
    {
        label: 'Elixir',
        mime: 'text/x-elixir',
        extension: 'ex',
    },
];

const TEXT_PREVIEW_LANGUAGES = new Map(
    Object.entries({
        txt: 'plaintext',
        text: 'plaintext',
        log: 'plaintext',
        nfo: 'plaintext',
        csv: 'plaintext',
        tsv: 'plaintext',
        srt: 'plaintext',
        vtt: 'plaintext',
        rst: 'plaintext',
        gitignore: 'plaintext',
        gitattributes: 'plaintext',
        dockerignore: 'plaintext',
        npmignore: 'plaintext',
        md: 'markdown',
        markdown: 'markdown',
        mdx: 'markdown',
        html: 'xml',
        htm: 'xml',
        xhtml: 'xml',
        xml: 'xml',
        plist: 'xml',
        vue: 'xml',
        svelte: 'xml',
        css: 'css',
        scss: 'scss',
        sass: 'scss',
        less: 'less',
        js: 'javascript',
        mjs: 'javascript',
        cjs: 'javascript',
        jsx: 'javascript',
        ts: 'typescript',
        mts: 'typescript',
        cts: 'typescript',
        tsx: 'typescript',
        json: 'json',
        jsonc: 'json',
        json5: 'json',
        yml: 'yaml',
        yaml: 'yaml',
        toml: 'ini',
        ini: 'ini',
        cfg: 'ini',
        conf: 'ini',
        env: 'ini',
        properties: 'properties',
        editorconfig: 'ini',
        py: 'python',
        rb: 'ruby',
        php: 'php',
        java: 'java',
        kt: 'kotlin',
        kts: 'kotlin',
        swift: 'swift',
        go: 'go',
        rs: 'rust',
        c: 'c',
        h: 'c',
        cpp: 'cpp',
        cc: 'cpp',
        cxx: 'cpp',
        hpp: 'cpp',
        cs: 'csharp',
        dart: 'dart',
        scala: 'scala',
        groovy: 'groovy',
        gradle: 'groovy',
        pl: 'perl',
        pm: 'perl',
        lua: 'lua',
        r: 'r',
        ex: 'elixir',
        exs: 'elixir',
        hs: 'haskell',
        clj: 'clojure',
        sql: 'sql',
        graphql: 'graphql',
        gql: 'graphql',
        tex: 'latex',
        diff: 'diff',
        patch: 'diff',
        sh: 'bash',
        bash: 'bash',
        zsh: 'bash',
        bat: 'dos',
        cmd: 'dos',
        ps1: 'powershell',
        psm1: 'powershell',
        psd1: 'powershell',
        vb: 'vbnet',
        proto: 'protobuf',
        cmake: 'cmake',
        mk: 'makefile',
        dockerfile: 'dockerfile',
    }),
);

const TEXT_PREVIEW_NAMES = new Map(
    Object.entries({
        dockerfile: 'dockerfile',
        makefile: 'makefile',
        license: 'plaintext',
        readme: 'plaintext',
        changelog: 'plaintext',
    }),
);

const TEXT_MIME_TYPES = [
    'application/json',
    'application/javascript',
    'application/x-javascript',
    'application/ecmascript',
    'application/typescript',
    'application/xml',
    'application/x-sh',
    'application/toml',
    'application/x-toml',
    'application/yaml',
    'application/x-yaml',
    'application/sql',
    'application/graphql',
    'application/x-httpd-php',
    'application/x-php',
];

export const TEXT_PREVIEW_MAX_SIZE = 2 * 1024 * 1024;

export const textPreviewLanguage = (name: string, mimeType: string) => {
    const lowerName = name.toLowerCase();
    const dot = lowerName.lastIndexOf('.');

    return (
        TEXT_PREVIEW_NAMES.get(lowerName) ??
        TEXT_PREVIEW_LANGUAGES.get(dot === -1 ? '' : lowerName.slice(dot + 1)) ??
        (mimeType.startsWith('text/') ||
        TEXT_MIME_TYPES.includes(mimeType) ||
        (mimeType.startsWith('application/') &&
            (mimeType.endsWith('+json') || mimeType.endsWith('+xml')))
            ? 'plaintext'
            : null)
    );
};

export const isVideoFile = (file: { mimeType: string; metadata?: { duration?: number } | null }) =>
    file.mimeType.startsWith('video/') &&
    (file.mimeType !== 'video/mp2t' || !!file.metadata?.duration);

export const CODE_FILE_EXTENSIONS = [...TEXT_PREVIEW_LANGUAGES]
    .filter(([, language]) => language !== 'plaintext')
    .map(([extension]) => `.${extension}`);

export const DOCUMENT_FILE_MIME_TYPES = [
    'application/pdf',
    'application/msword',
    'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
    'application/vnd.ms-excel',
    'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
    'application/vnd.ms-powerpoint',
    'application/vnd.openxmlformats-officedocument.presentationml.presentation',
    'application/vnd.ms-outlook',
    'application/vnd.ms-project',
    'application/vnd.ms-works',
    'application/vnd.oasis.opendocument.text',
    'application/vnd.oasis.opendocument.spreadsheet',
    'application/vnd.oasis.opendocument.presentation',
    'application/vnd.oasis.opendocument.graphics',
    'text/csv',
    'text/rtf',
    'text/markdown',
    'application/xml',
    'application/vnd.api+json',
    'application/x-yaml',
    'application/epub+zip',
    'application/postscript',
    'application/x-dvi',
    'application/vnd.palm',
];

export const ARCHIVE_FILE_MIME_TYPES = [
    'application/zip',
    'application/x-zip-compressed',
    'multipart/x-zip',
    'application/x-rar-compressed',
    'application/x-tar',
    'application/gzip',
    'application/x-gzip',
    'application/x-bzip',
    'application/x-bzip2',
    'application/x-lzma',
    'application/x-xz',
    'application/x-7z-compressed',
    'application/java-archive',
    'application/vnd.ms-cab-compressed',
    'application/x-cpio',
    'application/x-compress',
    'application/x-compressed',
];

export const isPdf = (mimeType: string) => mimeType === 'application/pdf';

export const isProcessableImage = (mimeType: string) =>
    mimeType.startsWith('image/') && !['image/gif', 'image/svg+xml'].includes(mimeType);
