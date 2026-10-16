import {
    type ModelUrls,
    PaddleOcrService,
    V5_ARABIC_MOBILE_MODEL,
    V5_CYRILLIC_MOBILE_MODEL,
    V5_GREEK_MOBILE_MODEL,
    V5_KOREAN_MOBILE_MODEL,
    V5_THAI_MOBILE_MODEL,
    V6_SMALL_MODEL,
    V6_TINY_MODEL,
} from 'ppu-paddle-ocr';

import { downloadModelFile, getSessionOptions, useAIModel } from './models';

const MIN_CONFIDENCE = 0.7;
const GOOD_CONFIDENCE = 0.85;
const READ_CONFIDENCE = 0.3;

const BASE_MODELS = { tiny: V6_TINY_MODEL, small: V6_SMALL_MODEL };

const SCRIPT_MODELS: Record<string, ModelUrls> = {
    cyrillic: V5_CYRILLIC_MOBILE_MODEL,
    arabic: V5_ARABIC_MOBILE_MODEL,
    korean: V5_KOREAN_MOBILE_MODEL,
    thai: V5_THAI_MOBILE_MODEL,
    greek: V5_GREEK_MOBILE_MODEL,
};

type Box = { x: number; y: number; width: number; height: number };
type Item = { text: string; confidence: number; box: Box };
type Line = { text: string; confidence: number; box: Box };

const loadService = async (model: ModelUrls) => {
    const [detection, recognition, charactersDictionary] = await Promise.all(
        [model.detection, model.recognition, model.charactersDictionary].map((url) =>
            downloadModelFile(
                url,
                `paddle/${new URL(url).pathname.split('/').slice(-3).join('_')}`,
            ),
        ),
    );

    const service = new PaddleOcrService({
        model: { detection, recognition, charactersDictionary },
        session: { ...getSessionOptions() },
    });

    await service.initialize();
    return service;
};

const useOcrService = <T>(
    name: string,
    model: ModelUrls,
    fn: (service: PaddleOcrService) => Promise<T>,
) =>
    useAIModel(
        `ocr:${name}`,
        () => loadService(model),
        (service) => service.destroy(),
        fn,
    );

const groupItems = (items: Item[]) => {
    const groups: Item[][] = [];
    let line: Item[] = [];
    let lineY = items[0]?.box.y ?? 0;
    let averageHeight = items[0]?.box.height ?? 0;

    for (const item of items) {
        if (Math.abs(item.box.y - lineY) < averageHeight / 2) {
            line.push(item);
            averageHeight = (averageHeight * (line.length - 1) + item.box.height) / line.length;
        } else {
            groups.push(line.sort((a, b) => a.box.x - b.box.x));
            line = [item];
            lineY = item.box.y;
            averageHeight = item.box.height;
        }
    }

    if (line.length) groups.push(line.sort((a, b) => a.box.x - b.box.x));
    return groups;
};

const toLines = (groups: Item[][]): Line[] =>
    groups.map((items) => {
        const xs = items.flatMap((item) => [item.box.x, item.box.x + item.box.width]);
        const ys = items.flatMap((item) => [item.box.y, item.box.y + item.box.height]);

        return {
            text: items
                .map((item) => item.text)
                .join(' ')
                .trim(),
            confidence: items.reduce((sum, item) => sum + item.confidence, 0) / items.length,
            box: {
                x: Math.min(...xs),
                y: Math.min(...ys),
                width: Math.max(...xs) - Math.min(...xs),
                height: Math.max(...ys) - Math.min(...ys),
            },
        };
    });

const readImage = async (service: PaddleOcrService, image: ArrayBuffer) => {
    const internal = service as any;
    const canvas = await internal.platform.canvas.prepareCanvas(image);
    const boxes: Box[] = (await internal.detector.run(canvas)).filter(
        (box: Box) => box.width > 0 && box.height > 0,
    );

    if (!boxes.length) return { boxes, lines: [] as Line[] };

    const items: Item[] = await internal.recognitor.run(
        canvas,
        boxes,
        internal.options.recognition?.charactersDictionary,
        internal.options.recognition?.strategy ?? 'per-line',
        { minimumConfidence: READ_CONFIDENCE },
    );

    return { boxes, lines: toLines(groupItems(items)) };
};

const letters = (text: string) => text.match(/[\p{L}\p{N}]/gu)?.length ?? 0;
const area = (box: Box) => box.width * box.height;
const lineScore = (line: Line) => letters(line.text) * line.confidence;
const totalScore = (lines: Line[]) =>
    lines
        .filter((line) => line.confidence >= MIN_CONFIDENCE)
        .reduce((sum, line) => sum + lineScore(line), 0);

const overlap = (a: Box, b: Box) =>
    Math.max(0, Math.min(a.y + a.height, b.y + b.height) - Math.max(a.y, b.y)) /
    Math.max(1, Math.min(a.height, b.height));

const isGood = (line: Line) =>
    line.confidence >= GOOD_CONFIDENCE &&
    letters(line.text) >= 0.45 * (line.box.width / Math.max(1, line.box.height));

const fixRightToLeft = (text: string) =>
    [...text]
        .reverse()
        .join('')
        .replace(/[0-9٠-٩.,:/-]+/g, (run) => [...run].reverse().join(''));

const recognizeWithScripts = async (image: ArrayBuffer, lines: Line[]) => {
    let best: { score: number; lines: Line[] } = { score: 0, lines: [] };

    for (const [name, model] of Object.entries(SCRIPT_MODELS)) {
        let scriptLines = await useOcrService(
            `script:${name}`,
            model,
            async (service) => (await readImage(service, image)).lines,
        );

        if (name === 'arabic')
            scriptLines = scriptLines.map((line) => ({ ...line, text: fixRightToLeft(line.text) }));

        const score = totalScore(scriptLines);
        if (score > best.score) best = { score, lines: scriptLines };
    }

    if (best.score <= totalScore(lines) * 0.5) return lines;

    const merged = lines.map((line) => {
        const rival = best.lines
            .filter((other) => overlap(line.box, other.box) > 0.5)
            .sort((a, b) => lineScore(b) - lineScore(a))[0];

        return rival && lineScore(rival) > lineScore(line) ? rival : line;
    });

    return [
        ...merged,
        ...best.lines.filter((line) => !merged.some((other) => overlap(other.box, line.box) > 0.5)),
    ].sort((a, b) => a.box.y - b.box.y);
};

export const recognizeImageText = async (
    image: ArrayBuffer,
    size: { width: number; height: number },
    tier: 'tiny' | 'small',
) => {
    const { boxes, lines } = await useOcrService(`base:${tier}`, BASE_MODELS[tier], (service) =>
        readImage(service, image),
    );

    const good = lines.filter(isGood);
    const unread = boxes.filter(
        (box) =>
            box.width >= 2.5 * box.height &&
            !good.some(
                (line) =>
                    overlap(line.box, box) > 0.5 &&
                    box.x + box.width > line.box.x &&
                    box.x < line.box.x + line.box.width,
            ),
    );

    const finalLines =
        unread.length &&
        unread.reduce((sum, box) => sum + area(box), 0) >= 0.01 * size.width * size.height
            ? await recognizeWithScripts(image, lines)
            : lines;

    return finalLines
        .filter((line) => line.confidence >= MIN_CONFIDENCE && letters(line.text))
        .map((line) => line.text)
        .join('\n');
};
