import { randomUUID } from 'node:crypto';
import { once } from 'node:events';
import { createWriteStream, promises as fsp, type WriteStream } from 'node:fs';
import type { Readable } from 'node:stream';

import busboy from 'busboy';
import type { H3Event } from 'h3';
import { join } from 'pathe';

export type UploadData = Uint8Array | { path: string; size: number };

export interface UploadFormFile {
    name: string;
    type: string;
    size: number;
    data: UploadData;
}

export interface UploadForm {
    get: (name: string) => string | null;
    file: UploadFormFile | null;
}

interface UploadFormOptions {
    field: string;
    memoryLimit: number;
    spill?: boolean;
    maxSize?: number | null;
    maxSizeError?: () => Error;
}

const MAX_FIELD_SIZE = 64 * 1024;

export const uploadDataSize = (data: UploadData) =>
    data instanceof Uint8Array ? data.byteLength : data.size;

export const removeUploadForm = async (form: UploadForm | null) => {
    if (form?.file && !(form.file.data instanceof Uint8Array))
        await fsp.rm(form.file.data.path, { force: true });
};

const requestTooLarge = () => apiError(413, 'errorsSystem.requestTooLarge');

const readFile = async (
    stream: Readable,
    { memoryLimit, spill, maxSize, maxSizeError = requestTooLarge }: UploadFormOptions,
) => {
    const chunks: Buffer[] = [];
    let size = 0;
    let output: WriteStream | null = null;
    let path = '';

    try {
        for await (const chunk of stream as AsyncIterable<Buffer>) {
            size += chunk.length;

            if (maxSize != null && size > maxSize) throw maxSizeError();

            if (output) {
                if (!output.write(chunk)) await once(output, 'drain');
                continue;
            }

            chunks.push(chunk);

            if (size > memoryLimit) {
                if (!spill) throw requestTooLarge();

                path = join(dataDirectory, 'temp', 'incoming', `${randomUUID()}.part`);
                await fsp.mkdir(join(path, '..'), { recursive: true });

                output = createWriteStream(path);
                output.write(Buffer.concat(chunks));
                chunks.length = 0;
            }
        }

        if (!output) return Buffer.concat(chunks);

        output.end();
        await once(output, 'finish');

        return { path, size };
    } catch (error) {
        if (output) {
            output.destroy();
            await fsp.rm(path, { force: true });
        }

        throw error;
    }
};

export const readUploadForm = (event: H3Event, options: UploadFormOptions) =>
    new Promise<UploadForm>((resolve, reject) => {
        const request = event.node.req;
        const fields = new Map<string, string>();
        const form: UploadForm = { get: (name) => fields.get(name) ?? null, file: null };

        let parser: busboy.Busboy;

        try {
            parser = busboy({
                headers: request.headers,
                limits: { files: 1, fields: 32, fieldSize: MAX_FIELD_SIZE },
            });
        } catch {
            resolve(form);
            return;
        }

        let reading: Promise<unknown> = Promise.resolve();
        let isFailed = false;

        const fail = (error: unknown) => {
            if (isFailed) return;
            isFailed = true;

            request.unpipe(parser);
            request.resume();
            reading.then(() => removeUploadForm(form)).catch(() => null);
            reject(error);
        };

        parser.on('field', (name, value) => fields.set(name, value));

        parser.on('file', (name, stream, info) => {
            if (name !== options.field || isFailed) {
                stream.resume();
                return;
            }

            reading = readFile(stream, options).then((data) => {
                form.file = {
                    name: info.filename ?? '',
                    type: info.mimeType,
                    size: uploadDataSize(data),
                    data,
                };
            }, fail);
        });

        parser.on('close', () =>
            reading.then(() => {
                if (!isFailed) resolve(form);
            }),
        );

        parser.on('error', fail);
        request.on('error', fail);
        request.on('aborted', () => fail(apiError(400, 'errorsSystem.requestAborted')));

        request.pipe(parser);
    });
