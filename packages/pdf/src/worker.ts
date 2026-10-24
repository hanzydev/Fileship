import { closeSync, fstatSync, openSync, readFileSync, readSync } from 'node:fs';
import { createRequire } from 'node:module';
import { parentPort } from 'node:worker_threads';

import type { PdfPreview, PdfRequest, PdfResponse } from './types';

import { init, type WrappedPdfiumModule } from '@embedpdf/pdfium';

const FPDF_ANNOT = 0x01;
const FPDF_REVERSE_BYTE_ORDER = 0x10;
const MAX_FILE_SIZE = 0xffffffff;

const port = parentPort!;

let pdfium: Promise<WrappedPdfiumModule> | undefined;

const loadPdfium = () => {
    pdfium ??= (async () => {
        const wasm = readFileSync(
            createRequire(import.meta.url).resolve('@embedpdf/pdfium/pdfium.wasm'),
        );
        const module = await init({
            wasmBinary: wasm.buffer.slice(wasm.byteOffset, wasm.byteOffset + wasm.byteLength),
        });

        module.PDFiumExt_Init();

        return module;
    })();

    return pdfium;
};

const openDocument = (module: WrappedPdfiumModule, path: string) => {
    const { pdfium } = module;
    const fd = openSync(path, 'r');
    const { size } = fstatSync(fd);

    const getBlock = pdfium.addFunction(
        (_param: number, position: number, buffer: number, length: number) =>
            readSync(fd, pdfium.HEAPU8, buffer, length, position >>> 0) === length ? 1 : 0,
        'iiiii',
    );
    const access = pdfium.wasmExports.malloc(12);

    pdfium.setValue(access, size, 'i32');
    pdfium.setValue(access + 4, getBlock, 'i32');
    pdfium.setValue(access + 8, 0, 'i32');

    const document = size && size <= MAX_FILE_SIZE ? module.FPDF_LoadCustomDocument(access, '') : 0;

    return {
        document,
        close() {
            if (document) module.FPDF_CloseDocument(document);

            pdfium.wasmExports.free(access);
            pdfium.removeFunction(getBlock);
            closeSync(fd);
        },
    };
};

const readVersion = (module: WrappedPdfiumModule, document: number) => {
    const pointer = module.pdfium.wasmExports.malloc(4);
    const version = module.FPDF_GetFileVersion(document, pointer)
        ? module.pdfium.getValue(pointer, 'i32')
        : null;

    module.pdfium.wasmExports.free(pointer);

    return version;
};

const renderPage = (module: WrappedPdfiumModule, document: number, maxSize: number) => {
    const page = module.FPDF_LoadPage(document, 0);
    if (!page) return null;

    try {
        const pageWidth = module.FPDF_GetPageWidthF(page);
        const pageHeight = module.FPDF_GetPageHeightF(page);
        if (!(pageWidth > 0 && pageHeight > 0)) return null;

        const scale = Math.min(maxSize / pageWidth, maxSize / pageHeight);
        const width = Math.max(1, Math.round(pageWidth * scale));
        const height = Math.max(1, Math.round(pageHeight * scale));
        const bitmap = module.FPDFBitmap_Create(width, height, 0);

        try {
            module.FPDFBitmap_FillRect(bitmap, 0, 0, width, height, 0xffffffff);
            module.FPDF_RenderPageBitmap(
                bitmap,
                page,
                0,
                0,
                width,
                height,
                0,
                FPDF_ANNOT | FPDF_REVERSE_BYTE_ORDER,
            );

            const buffer = module.FPDFBitmap_GetBuffer(bitmap);
            const data = module.pdfium.HEAPU8.slice(buffer, buffer + width * height * 4);

            return { width, height, data };
        } finally {
            module.FPDFBitmap_Destroy(bitmap);
        }
    } finally {
        module.FPDF_ClosePage(page);
    }
};

const preview = async (path: string, maxSize: number): Promise<PdfPreview | null> => {
    const module = await loadPdfium();
    const { document, close } = openDocument(module, path);

    try {
        if (!document) return null;

        return {
            pages: module.FPDF_GetPageCount(document),
            version: readVersion(module, document),
            image: maxSize ? renderPage(module, document, maxSize) : null,
        };
    } finally {
        close();
    }
};

const send = (response: PdfResponse) =>
    port.postMessage(
        response,
        response.type === 'result' && response.value?.image
            ? [response.value.image.data.buffer as ArrayBuffer]
            : [],
    );

port.on('message', async ({ path, maxSize }: PdfRequest) => {
    try {
        send({ type: 'result', value: await preview(path, maxSize) });
    } catch (error: any) {
        send({ type: 'error', message: error?.message ?? String(error) });
    }
});
