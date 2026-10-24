export type PdfPreview = {
    pages: number;
    version: number | null;
    image: { width: number; height: number; data: Uint8Array } | null;
};

export type PdfRequest = {
    path: string;
    maxSize: number;
};

export type PdfResponse =
    { type: 'result'; value: PdfPreview | null } | { type: 'error'; message: string };
