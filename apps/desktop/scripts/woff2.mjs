import { brotliDecompressSync } from 'node:zlib';

const KNOWN_TAGS = [
    'cmap',
    'head',
    'hhea',
    'hmtx',
    'maxp',
    'name',
    'OS/2',
    'post',
    'cvt ',
    'fpgm',
    'glyf',
    'loca',
    'prep',
    'CFF ',
    'VORG',
    'EBDT',
    'EBLC',
    'gasp',
    'hdmx',
    'kern',
    'LTSH',
    'PCLT',
    'VDMX',
    'vhea',
    'vmtx',
    'BASE',
    'GDEF',
    'GPOS',
    'GSUB',
    'EBSC',
    'JSTF',
    'MATH',
    'CBDT',
    'CBLC',
    'COLR',
    'CPAL',
    'SVG ',
    'sbix',
    'acnt',
    'avar',
    'bdat',
    'bloc',
    'bsln',
    'cvar',
    'fdsc',
    'feat',
    'fmtx',
    'fvar',
    'gvar',
    'hsty',
    'just',
    'lcar',
    'mort',
    'morx',
    'opbd',
    'prop',
    'trak',
    'Zapf',
    'Silf',
    'Glat',
    'Gloc',
    'Feat',
    'Sill',
];

const ON_CURVE = 0x01;
const X_SHORT = 0x02;
const Y_SHORT = 0x04;
const X_SAME = 0x10;
const Y_SAME = 0x20;
const OVERLAP_SIMPLE = 0x40;

const ARGS_ARE_WORDS = 0x0001;
const HAVE_SCALE = 0x0008;
const MORE_COMPONENTS = 0x0020;
const HAVE_XY_SCALE = 0x0040;
const HAVE_TWO_BY_TWO = 0x0080;
const HAVE_INSTRUCTIONS = 0x0100;

class Reader {
    constructor(buffer, offset = 0, end = buffer.length) {
        this.buffer = buffer;
        this.offset = offset;
        this.end = end;
    }

    take(length) {
        if (this.offset + length > this.end) throw new Error('The WOFF2 font is truncated.');
        const start = this.offset;
        this.offset += length;
        return start;
    }

    u8() {
        return this.buffer.readUInt8(this.take(1));
    }

    u16() {
        return this.buffer.readUInt16BE(this.take(2));
    }

    i16() {
        return this.buffer.readInt16BE(this.take(2));
    }

    u32() {
        return this.buffer.readUInt32BE(this.take(4));
    }

    bytes(length) {
        const start = this.take(length);
        return this.buffer.subarray(start, start + length);
    }

    base128() {
        let value = 0;
        for (let index = 0; index < 5; index++) {
            const byte = this.u8();
            if (index === 0 && byte === 0x80) throw new Error('Invalid UIntBase128 value.');
            value = value * 128 + (byte & 0x7f);
            if (!(byte & 0x80)) return value;
        }
        throw new Error('Invalid UIntBase128 value.');
    }

    u255() {
        const code = this.u8();
        if (code === 253) return this.u16();
        if (code === 255) return this.u8() + 253;
        if (code === 254) return this.u8() + 253 * 2;
        return code;
    }

    stream(length) {
        const start = this.take(length);
        return new Reader(this.buffer, start, start + length);
    }
}

class Writer {
    constructor() {
        this.chunks = [];
        this.length = 0;
    }

    push(buffer) {
        this.chunks.push(buffer);
        this.length += buffer.length;
    }

    u8(value) {
        this.push(Buffer.from([value & 0xff]));
    }

    u16(value) {
        const buffer = Buffer.alloc(2);
        buffer.writeUInt16BE(value & 0xffff);
        this.push(buffer);
    }

    i16(value) {
        const buffer = Buffer.alloc(2);
        buffer.writeInt16BE(value);
        this.push(buffer);
    }

    u32(value) {
        const buffer = Buffer.alloc(4);
        buffer.writeUInt32BE(value);
        this.push(buffer);
    }

    pad(alignment) {
        while (this.length % alignment) this.u8(0);
    }

    result() {
        return Buffer.concat(this.chunks, this.length);
    }
}

const withSign = (flag, value) => (flag & 1 ? value : -value);

const decodePoints = (flags, glyphs, count) => {
    const points = [];
    let x = 0;
    let y = 0;

    for (let index = 0; index < count; index++) {
        const flag = flags.u8();
        const onCurve = !(flag >> 7);
        const code = flag & 0x7f;
        let dx;
        let dy;

        if (code < 10) {
            dx = 0;
            dy = withSign(code, ((code & 14) << 7) + glyphs.u8());
        } else if (code < 20) {
            dx = withSign(code, (((code - 10) & 14) << 7) + glyphs.u8());
            dy = 0;
        } else if (code < 84) {
            const b0 = code - 20;
            const b1 = glyphs.u8();
            dx = withSign(code, 1 + (b0 & 0x30) + (b1 >> 4));
            dy = withSign(code >> 1, 1 + ((b0 & 0x0c) << 2) + (b1 & 0x0f));
        } else if (code < 120) {
            const b0 = code - 84;
            dx = withSign(code, 1 + (Math.floor(b0 / 12) << 8) + glyphs.u8());
            dy = withSign(code >> 1, 1 + (((b0 % 12) >> 2) << 8) + glyphs.u8());
        } else if (code < 124) {
            const b1 = glyphs.u8();
            const b2 = glyphs.u8();
            const b3 = glyphs.u8();
            dx = withSign(code, (b1 << 4) + (b2 >> 4));
            dy = withSign(code >> 1, ((b2 & 0x0f) << 8) + b3);
        } else {
            const b1 = glyphs.u8();
            const b2 = glyphs.u8();
            const b3 = glyphs.u8();
            const b4 = glyphs.u8();
            dx = withSign(code, (b1 << 8) + b2);
            dy = withSign(code >> 1, (b3 << 8) + b4);
        }

        x += dx;
        y += dy;
        points.push({ x, y, onCurve });
    }

    return points;
};

const writeSimpleGlyph = (writer, contours, points, bbox, instructions, hasOverlap) => {
    writer.i16(contours.length);
    for (const value of bbox) writer.i16(value);

    let end = -1;
    for (const count of contours) {
        end += count;
        writer.u16(end);
    }

    writer.u16(instructions.length);
    writer.push(instructions);

    const xs = new Writer();
    const ys = new Writer();
    let previousX = 0;
    let previousY = 0;

    for (const [index, point] of points.entries()) {
        let flag = point.onCurve ? ON_CURVE : 0;
        if (index === 0 && hasOverlap) flag |= OVERLAP_SIMPLE;

        const dx = point.x - previousX;
        const dy = point.y - previousY;

        if (dx === 0) {
            flag |= X_SAME;
        } else if (dx > -256 && dx < 256) {
            flag |= X_SHORT | (dx > 0 ? X_SAME : 0);
            xs.u8(Math.abs(dx));
        } else {
            xs.i16(dx);
        }

        if (dy === 0) {
            flag |= Y_SAME;
        } else if (dy > -256 && dy < 256) {
            flag |= Y_SHORT | (dy > 0 ? Y_SAME : 0);
            ys.u8(Math.abs(dy));
        } else {
            ys.i16(dy);
        }

        writer.u8(flag);
        previousX = point.x;
        previousY = point.y;
    }

    writer.push(xs.result());
    writer.push(ys.result());
};

const compositeLength = (stream) => {
    const start = stream.offset;
    let hasInstructions = false;
    let flags = MORE_COMPONENTS;

    while (flags & MORE_COMPONENTS) {
        flags = stream.u16();
        hasInstructions ||= Boolean(flags & HAVE_INSTRUCTIONS);
        let size = 2 + (flags & ARGS_ARE_WORDS ? 4 : 2);
        if (flags & HAVE_SCALE) size += 2;
        else if (flags & HAVE_XY_SCALE) size += 4;
        else if (flags & HAVE_TWO_BY_TWO) size += 8;
        stream.take(size);
    }

    return { length: stream.offset - start, hasInstructions };
};

const reconstructGlyf = (data, indexFormat) => {
    const header = new Reader(data);
    header.u16();
    const optionFlags = header.u16();
    const numGlyphs = header.u16();
    header.u16();
    const sizes = Array.from({ length: 7 }, () => header.u32());
    const [
        contourStream,
        pointStream,
        flagStream,
        glyphStream,
        compositeStream,
        bboxStream,
        instructionStream,
    ] = sizes.map((size) => header.stream(size));
    const overlaps = optionFlags & 1 ? header.bytes((numGlyphs + 7) >> 3) : null;
    const bboxBitmap = bboxStream.bytes(((numGlyphs + 31) >> 5) << 2);

    const glyf = new Writer();
    const offsets = [];
    const xMins = [];

    for (let glyph = 0; glyph < numGlyphs; glyph++) {
        offsets.push(glyf.length);
        const contours = contourStream.i16();
        const hasBbox = Boolean(bboxBitmap[glyph >> 3] & (0x80 >> (glyph & 7)));

        if (contours === 0) {
            if (hasBbox) throw new Error('An empty glyph has a bounding box.');
            xMins.push(0);
            continue;
        }

        if (contours === -1) {
            if (!hasBbox) throw new Error('A composite glyph has no bounding box.');
            const bbox = [bboxStream.i16(), bboxStream.i16(), bboxStream.i16(), bboxStream.i16()];
            const start = compositeStream.offset;
            const { length, hasInstructions } = compositeLength(compositeStream);

            glyf.i16(-1);
            for (const value of bbox) glyf.i16(value);
            glyf.push(compositeStream.buffer.subarray(start, start + length));

            if (hasInstructions) {
                const size = glyphStream.u255();
                glyf.u16(size);
                glyf.push(instructionStream.bytes(size));
            }

            xMins.push(bbox[0]);
            glyf.pad(4);
            continue;
        }

        const counts = Array.from({ length: contours }, () => pointStream.u255());
        const total = counts.reduce((sum, count) => sum + count, 0);
        const points = decodePoints(flagStream, glyphStream, total);
        const instructions = instructionStream.bytes(glyphStream.u255());

        const bbox = hasBbox
            ? [bboxStream.i16(), bboxStream.i16(), bboxStream.i16(), bboxStream.i16()]
            : [
                  Math.min(...points.map((point) => point.x)),
                  Math.min(...points.map((point) => point.y)),
                  Math.max(...points.map((point) => point.x)),
                  Math.max(...points.map((point) => point.y)),
              ];
        const hasOverlap = Boolean(overlaps && overlaps[glyph >> 3] & (0x80 >> (glyph & 7)));

        writeSimpleGlyph(glyf, counts, points, bbox, instructions, hasOverlap);
        xMins.push(bbox[0]);
        glyf.pad(4);
    }

    offsets.push(glyf.length);

    const loca = new Writer();
    for (const offset of offsets) {
        if (indexFormat) loca.u32(offset);
        else loca.u16(offset / 2);
    }

    return { glyf: glyf.result(), loca: loca.result(), xMins };
};

const reconstructHmtx = (data, numGlyphs, numHMetrics, xMins) => {
    const reader = new Reader(data);
    const flags = reader.u8();
    const advances = Array.from({ length: numHMetrics }, () => reader.u16());
    const proportional = Array.from({ length: numHMetrics }, (_, index) =>
        flags & 1 ? xMins[index] : reader.i16(),
    );
    const monospaced = Array.from({ length: numGlyphs - numHMetrics }, (_, index) =>
        flags & 2 ? xMins[numHMetrics + index] : reader.i16(),
    );

    const writer = new Writer();
    for (let index = 0; index < numHMetrics; index++) {
        writer.u16(advances[index]);
        writer.i16(proportional[index]);
    }
    for (const value of monospaced) writer.i16(value);
    return writer.result();
};

const checksum = (buffer) => {
    let sum = 0;
    const padded = Buffer.concat([buffer, Buffer.alloc((4 - (buffer.length % 4)) % 4)]);
    for (let offset = 0; offset < padded.length; offset += 4)
        sum = (sum + padded.readUInt32BE(offset)) >>> 0;
    return sum;
};

export const decodeWoff2 = (input) => {
    const reader = new Reader(input);
    if (reader.u32() !== 0x774f4632) throw new Error('Not a WOFF2 font.');

    const flavor = reader.u32();
    if (flavor === 0x74746366) throw new Error('WOFF2 font collections are not supported.');
    reader.u32();
    const numTables = reader.u16();
    reader.u16();
    reader.u32();
    const compressedSize = reader.u32();
    reader.take(24);

    const entries = [];
    for (let index = 0; index < numTables; index++) {
        const flags = reader.u8();
        const tag =
            (flags & 0x3f) === 0x3f ? reader.bytes(4).toString('latin1') : KNOWN_TAGS[flags & 0x3f];
        const version = flags >> 6;
        const length = reader.base128();
        const isTransformed = tag === 'glyf' || tag === 'loca' ? version === 0 : version !== 0;
        const stored = isTransformed ? reader.base128() : length;
        entries.push({ tag, length, isTransformed, stored });
    }

    const data = brotliDecompressSync(reader.bytes(compressedSize));
    const tables = new Map();
    let offset = 0;
    for (const entry of entries) {
        tables.set(entry.tag, data.subarray(offset, offset + entry.stored));
        offset += entry.stored;
    }

    const output = new Map();
    for (const entry of entries)
        if (!entry.isTransformed) output.set(entry.tag, tables.get(entry.tag));

    const glyfEntry = entries.find((entry) => entry.tag === 'glyf');
    let xMins = [];
    if (glyfEntry?.isTransformed) {
        const indexFormat = output.get('head').readInt16BE(50);
        const rebuilt = reconstructGlyf(tables.get('glyf'), indexFormat);
        output.set('glyf', rebuilt.glyf);
        output.set('loca', rebuilt.loca);
        xMins = rebuilt.xMins;
    }

    const hmtxEntry = entries.find((entry) => entry.tag === 'hmtx');
    if (hmtxEntry?.isTransformed) {
        const numGlyphs = output.get('maxp').readUInt16BE(4);
        const numHMetrics = output.get('hhea').readUInt16BE(34);
        output.set('hmtx', reconstructHmtx(tables.get('hmtx'), numGlyphs, numHMetrics, xMins));
    }

    const head = Buffer.from(output.get('head'));
    head.writeUInt32BE(0, 8);
    output.set('head', head);

    const tags = [...output.keys()].sort();
    const searchRange = 2 ** Math.floor(Math.log2(tags.length)) * 16;
    const header = Buffer.alloc(12 + tags.length * 16);
    header.writeUInt32BE(flavor, 0);
    header.writeUInt16BE(tags.length, 4);
    header.writeUInt16BE(searchRange, 6);
    header.writeUInt16BE(Math.log2(searchRange / 16), 8);
    header.writeUInt16BE(tags.length * 16 - searchRange, 10);

    const body = new Writer();
    let position = header.length;
    for (const [index, tag] of tags.entries()) {
        const table = output.get(tag);
        const record = 12 + index * 16;
        header.write(tag, record, 4, 'latin1');
        header.writeUInt32BE(checksum(table), record + 4);
        header.writeUInt32BE(position, record + 8);
        header.writeUInt32BE(table.length, record + 12);
        body.push(table);
        body.pad(4);
        position = header.length + body.length;
    }

    const font = Buffer.concat([header, body.result()]);
    const headOffset = header.readUInt32BE(12 + tags.indexOf('head') * 16 + 8);
    font.writeUInt32BE((0xb1b0afba - checksum(font)) >>> 0, headOffset + 8);
    return font;
};
