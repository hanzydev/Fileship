import { readFileSync, writeFileSync } from 'node:fs';
import { dirname, join } from 'node:path';
import { fileURLToPath } from 'node:url';

const resources = join(dirname(fileURLToPath(import.meta.url)), '..', 'resources');
const RATE = 44_100;

const readWav = (path) => {
    const buffer = readFileSync(path);

    for (let offset = 12; offset < buffer.length;) {
        const id = buffer.toString('ascii', offset, offset + 4);
        const size = buffer.readUInt32LE(offset + 4);

        if (id === 'data') {
            return Float32Array.from(
                { length: size / 2 },
                (_, index) => buffer.readInt16LE(offset + 8 + index * 2) / 32768,
            );
        }

        offset += 8 + size + (size & 1);
    }

    throw new Error(`${path} has no data chunk`);
};

const writeWav = (path, samples) => {
    const buffer = Buffer.alloc(44 + samples.length * 2);

    buffer.write('RIFF', 0, 'ascii');
    buffer.writeUInt32LE(36 + samples.length * 2, 4);
    buffer.write('WAVEfmt ', 8, 'ascii');
    buffer.writeUInt32LE(16, 16);
    buffer.writeUInt16LE(1, 20);
    buffer.writeUInt16LE(1, 22);
    buffer.writeUInt32LE(RATE, 24);
    buffer.writeUInt32LE(RATE * 2, 28);
    buffer.writeUInt16LE(2, 32);
    buffer.writeUInt16LE(16, 34);
    buffer.write('data', 36, 'ascii');
    buffer.writeUInt32LE(samples.length * 2, 40);

    samples.forEach((sample, index) =>
        buffer.writeInt16LE(Math.round(Math.max(-1, Math.min(1, sample)) * 32767), 44 + index * 2),
    );

    writeFileSync(path, buffer);
};

const resample = (samples, factor) =>
    Float32Array.from({ length: Math.floor(samples.length / factor) }, (_, index) => {
        const position = index * factor;
        const base = Math.floor(position);
        const next = samples[base + 1] ?? 0;

        return samples[base] + (next - samples[base]) * (position - base);
    });

const fadeOut = (samples, length) =>
    samples.map((sample, index) =>
        index < samples.length - length ? sample : sample * ((samples.length - index) / length),
    );

const shutter = readWav(join(resources, 'shutter.wav'));
const peak = shutter.reduce((max, sample) => Math.max(max, Math.abs(sample)), 0);
const output = new Float32Array(Math.round(0.36 * RATE));

const mix = (samples, startMs, gain) => {
    const start = Math.round((startMs / 1000) * RATE);
    samples.forEach((sample, index) => {
        if (start + index < output.length) output[start + index] += sample * gain;
    });
};

const chime = (frequency, startMs, lengthMs, gain) => {
    const length = Math.round((lengthMs / 1000) * RATE);

    return mix(
        Float32Array.from({ length }, (_, index) => {
            const time = index / RATE;
            const attack = Math.min(1, index / (0.004 * RATE));
            const decay = Math.exp(-time * 14);
            const tone =
                Math.sin(2 * Math.PI * frequency * time) +
                0.25 * Math.sin(2 * Math.PI * frequency * 2 * time);

            return tone * attack * decay * Math.min(1, (length - index) / (0.01 * RATE));
        }),
        startMs,
        gain,
    );
};

mix(fadeOut(resample(shutter, 1.5), 400), 0, 0.55);
mix(fadeOut(resample(shutter, 2), 400), 85, 0.45);
chime(1318.5, 70, 290, 0.18);
chime(1975.5, 120, 240, 0.16);

const outputPeak = output.reduce((max, sample) => Math.max(max, Math.abs(sample)), 0);
writeWav(
    join(resources, 'uploaded.wav'),
    output.map((sample) => (sample / outputPeak) * peak),
);
