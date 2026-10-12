export interface IntegrationOptions {
    name: string;
    url: string;
    token: string;
    fields: Record<string, string | number>;
}

const shellQuote = (value: string | number) => `'${String(value).replaceAll("'", `'\\''`)}'`;

export const buildShareXConfig = ({ name, url, token, fields }: IntegrationOptions) =>
    JSON.stringify(
        {
            Version: '16.1.0',
            Name: name,
            DestinationType: 'ImageUploader, TextUploader, FileUploader',
            RequestMethod: 'POST',
            RequestURL: url,
            Headers: { Authorization: `Bearer ${token}` },
            URL: '{json:url}',
            ErrorMessage: '{json:message}',
            Body: 'MultipartFormData',
            FileFormName: 'file',
            Arguments: fields,
        },
        null,
        2,
    );

const platforms = {
    macos: {
        capture: ['screencapture -i "$file"'],
        copy: 'printf %s "$1" | pbcopy',
        notify: 'osascript -e "display notification \\"$1\\" with title \\"Fileship\\""',
    },
    linux: {
        capture: [
            'if command -v grim >/dev/null && command -v slurp >/dev/null; then',
            '        region=$(slurp) || exit 0',
            '        grim -g "$region" "$file"',
            '    elif command -v flameshot >/dev/null; then',
            '        flameshot gui --raw > "$file" || exit 0',
            '    elif command -v maim >/dev/null; then',
            '        maim -s "$file" || exit 0',
            '    elif command -v gnome-screenshot >/dev/null; then',
            '        gnome-screenshot -a -f "$file"',
            '    elif command -v spectacle >/dev/null; then',
            '        spectacle -rbno "$file"',
            '    else',
            "        echo 'Install grim and slurp, flameshot, maim, gnome-screenshot or spectacle.' >&2",
            '        exit 1',
            '    fi',
        ],
        copy: [
            'if command -v wl-copy >/dev/null; then',
            '        printf %s "$1" | wl-copy',
            '    elif command -v xclip >/dev/null; then',
            '        printf %s "$1" | xclip -selection clipboard',
            '    elif command -v xsel >/dev/null; then',
            '        printf %s "$1" | xsel -b',
            '    fi',
        ].join('\n'),
        notify: 'if command -v notify-send >/dev/null; then notify-send Fileship "$1"; fi',
    },
};

export const buildUnixScript = (
    platform: keyof typeof platforms,
    { url, token, fields }: IntegrationOptions,
) => {
    const { capture, copy, notify } = platforms[platform];

    return [
        '#!/bin/sh',
        'set -eu',
        '',
        `url=${shellQuote(url)}`,
        `token=${shellQuote(token)}`,
        '',
        'copy() {',
        `    ${copy}`,
        '}',
        '',
        'notify() {',
        `    ${notify}`,
        '}',
        '',
        'if [ "$#" -eq 0 ]; then',
        '    file="${TMPDIR:-/tmp}/fileship-$$.png"',
        `    trap 'rm -f "$file"' EXIT`,
        `    ${capture.join('\n')}`,
        '    [ -s "$file" ] || exit 0',
        '    set -- "$file"',
        'fi',
        '',
        'links=""',
        '',
        'for file in "$@"; do',
        '    response=$(curl -fsS -H "Authorization: Bearer $token" \\',
        ...Object.entries(fields).map(
            ([key, value]) => `        --form-string ${shellQuote(`${key}=${value}`)} \\`,
        ),
        '        -F "file=@$file;type=$(file --mime-type -b "$file")" "$url") || {',
        '        notify "Upload failed"',
        '        exit 1',
        '    }',
        `    link=$(printf %s "$response" | sed -n 's/.*"url":"\\([^"]*\\)".*/\\1/p')`,
        '    echo "$link"',
        '    links="${links:+$links }$link"',
        'done',
        '',
        'copy "$links"',
        'notify "Link copied to clipboard"',
        '',
    ].join('\n');
};
