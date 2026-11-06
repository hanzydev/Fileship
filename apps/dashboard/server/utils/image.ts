import sharp, { type SharpInput, type SharpOptions } from 'sharp';

export const MAX_IMAGE_PIXELS = 120_000_000;

export const openImage = (input: SharpInput, options?: SharpOptions) =>
    sharp(input, { limitInputPixels: MAX_IMAGE_PIXELS, ...options });

export const isPixelLimitError = (error: unknown) =>
    error instanceof Error && error.message.includes('pixel limit');
