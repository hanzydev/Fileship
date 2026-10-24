type Pdfjs = typeof import('pdfjs-dist/legacy/build/pdf.mjs');

let pdfjs: Promise<Pdfjs> | undefined;

export const loadPdfjs = () => {
    pdfjs ??= Promise.all([
        import('pdfjs-dist/legacy/build/pdf.mjs'),
        import('pdfjs-dist/legacy/build/pdf.worker.min.mjs?url'),
    ]).then(([module, worker]) => {
        module.GlobalWorkerOptions.workerSrc = worker.default;
        return module;
    });

    return pdfjs;
};

export const openPdf = async (url: string, range: boolean) => {
    const module = await loadPdfjs();
    const assets = `/_pdfjs/${module.version}`;

    return module.getDocument({
        url,
        cMapUrl: `${assets}/cmaps/`,
        iccUrl: `${assets}/iccs/`,
        standardFontDataUrl: `${assets}/standard_fonts/`,
        wasmUrl: `${assets}/wasm/`,
        disableRange: !range,
        disableAutoFetch: true,
        enableXfa: false,
    });
};
