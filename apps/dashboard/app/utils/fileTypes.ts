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
        label: 'SQL',
        mime: 'text/x-sql',
        extension: 'sql',
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

export const CODE_FILE_EXTENSIONS = TEXT_FILE_TYPES.slice(1).map((type) => `.${type.extension}`);

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
    'application/x-sh',
    'application/x-compress',
    'application/x-compressed',
];
