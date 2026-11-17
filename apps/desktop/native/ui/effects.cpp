#include "effects.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace fileship::ui {

namespace {

constexpr int MAX_RADIUS = 180;

constexpr int MUL_TABLE[] = {
    512, 512, 456, 512, 328, 456, 335, 512, 405, 328, 271, 456, 388, 335, 292, 512, 454, 405, 364, 328, 298, 271, 496,
    456, 420, 388, 360, 335, 312, 292, 273, 512, 482, 454, 428, 405, 383, 364, 345, 328, 312, 298, 284, 271, 259, 496,
    475, 456, 437, 420, 404, 388, 374, 360, 347, 335, 323, 312, 302, 292, 282, 273, 265, 512, 497, 482, 468, 454, 441,
    428, 417, 405, 394, 383, 373, 364, 354, 345, 337, 328, 320, 312, 305, 298, 291, 284, 278, 271, 265, 259, 507, 496,
    485, 475, 465, 456, 446, 437, 428, 420, 412, 404, 396, 388, 381, 374, 367, 360, 354, 347, 341, 335, 329, 323, 318,
    312, 307, 302, 297, 292, 287, 282, 278, 273, 269, 265, 261, 512, 505, 497, 489, 482, 475, 468, 461, 454, 447, 441,
    435, 428, 422, 417, 411, 405, 399, 394, 389, 383, 378, 373, 368, 364, 359, 354, 350, 345, 341, 337, 332, 328, 324,
    320, 316, 312, 309, 305, 301, 298, 294, 291, 287, 284, 281, 278, 274, 271, 268, 265, 262, 259, 257, 507, 501, 496,
    491, 485, 480, 475, 470, 465, 460, 456, 451, 446, 442, 437, 433, 428, 424, 420, 416, 412, 408, 404, 400, 396, 392,
    388, 385, 381, 377, 374, 370, 367, 363, 360, 357, 354, 350, 347, 344, 341, 338, 335, 332, 329, 326, 323, 320, 318,
    315, 312, 310, 307, 304, 302, 299, 297, 294, 292, 289, 287, 285, 282, 280, 278, 275, 273, 271, 269, 267, 265, 263,
    261, 259,
};

constexpr int SHG_TABLE[] = {
    9,  11, 12, 13, 13, 14, 14, 15, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18,
    18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
    20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
    21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22,
    22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23, 23,
    23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
    24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
};

struct Entry {
    double r = 0;
    double g = 0;
    double b = 0;
    double a = 0;
};

uint8_t clamped(double value) {
    if (!(value > 0)) return 0;
    if (value >= 255) return 255;
    return static_cast<uint8_t>(std::nearbyint(value));
}

double premultiply(double channel, double alpha) {
    return alpha == 255 ? channel : channel * alpha / 255;
}

}

void stackBlur(uint8_t* pixels, int width, int height, int radius) {
    if (radius <= 0 || width <= 0 || height <= 0) return;

    int div = radius + radius + 1;
    int widthMinus1 = width - 1;
    int heightMinus1 = height - 1;
    int radiusPlus1 = radius + 1;
    double sumFactor = radiusPlus1 * (radiusPlus1 + 1) / 2.0;
    double normalization = radius <= MAX_RADIUS ? MUL_TABLE[radius] / std::pow(2.0, SHG_TABLE[radius])
                                                : 1.0 / (radiusPlus1 * radiusPlus1);

    std::vector<Entry> stack(div);
    auto next = [&](int index) { return (index + 1) % div; };
    int stackEnd = radiusPlus1 % div;

    int yw = 0;
    int yi = 0;

    for (int y = 0; y < height; y++) {
        double rIn = 0, gIn = 0, bIn = 0, aIn = 0, rSum = 0, gSum = 0, bSum = 0, aSum = 0;
        double pa = pixels[yi + 3];
        double pr = premultiply(pixels[yi], pa);
        double pg = premultiply(pixels[yi + 1], pa);
        double pb = premultiply(pixels[yi + 2], pa);
        double rOut = radiusPlus1 * pr, gOut = radiusPlus1 * pg, bOut = radiusPlus1 * pb, aOut = radiusPlus1 * pa;
        rSum += sumFactor * pr;
        gSum += sumFactor * pg;
        bSum += sumFactor * pb;
        aSum += sumFactor * pa;

        int cursor = 0;
        for (int i = 0; i < radiusPlus1; i++) {
            stack[cursor] = {pr, pg, pb, pa};
            cursor = next(cursor);
        }

        for (int i = 1; i < radiusPlus1; i++) {
            int p = yi + ((widthMinus1 < i ? widthMinus1 : i) << 2);
            pa = pixels[p + 3];
            int rbs = radiusPlus1 - i;
            pr = premultiply(pixels[p], pa);
            pg = premultiply(pixels[p + 1], pa);
            pb = premultiply(pixels[p + 2], pa);
            stack[cursor] = {pr, pg, pb, pa};
            rSum += pr * rbs;
            gSum += pg * rbs;
            bSum += pb * rbs;
            aSum += pa * rbs;
            rIn += pr;
            gIn += pg;
            bIn += pb;
            aIn += pa;
            cursor = next(cursor);
        }

        int stackIn = 0;
        int stackOut = stackEnd;

        for (int x = 0; x < width; x++) {
            pixels[yi] = clamped(std::floor(rSum * normalization));
            pixels[yi + 1] = clamped(std::floor(gSum * normalization));
            pixels[yi + 2] = clamped(std::floor(bSum * normalization));
            pixels[yi + 3] = clamped(std::floor(aSum * normalization));

            rSum -= rOut;
            gSum -= gOut;
            bSum -= bOut;
            aSum -= aOut;

            rOut -= stack[stackIn].r;
            gOut -= stack[stackIn].g;
            bOut -= stack[stackIn].b;
            aOut -= stack[stackIn].a;

            int offset = x + radius + 1;
            int p = (yw + (offset < widthMinus1 ? offset : widthMinus1)) << 2;
            pa = pixels[p + 3];
            stack[stackIn] = {premultiply(pixels[p], pa), premultiply(pixels[p + 1], pa), premultiply(pixels[p + 2], pa),
                              pa};
            rIn += stack[stackIn].r;
            gIn += stack[stackIn].g;
            bIn += stack[stackIn].b;
            aIn += stack[stackIn].a;

            rSum += rIn;
            gSum += gIn;
            bSum += bIn;
            aSum += aIn;

            stackIn = next(stackIn);

            pr = stack[stackOut].r;
            pg = stack[stackOut].g;
            pb = stack[stackOut].b;
            pa = stack[stackOut].a;
            rOut += pr;
            gOut += pg;
            bOut += pb;
            aOut += pa;
            rIn -= pr;
            gIn -= pg;
            bIn -= pb;
            aIn -= pa;

            stackOut = next(stackOut);
            yi += 4;
        }

        yw += width;
    }

    for (int x = 0; x < width; x++) {
        double rIn = 0, gIn = 0, bIn = 0, aIn = 0, rSum = 0, gSum = 0, bSum = 0, aSum = 0;
        yi = x << 2;
        double pr = pixels[yi], pg = pixels[yi + 1], pb = pixels[yi + 2], pa = pixels[yi + 3];
        double rOut = radiusPlus1 * pr, gOut = radiusPlus1 * pg, bOut = radiusPlus1 * pb, aOut = radiusPlus1 * pa;
        rSum += sumFactor * pr;
        gSum += sumFactor * pg;
        bSum += sumFactor * pb;
        aSum += sumFactor * pa;

        int cursor = 0;
        for (int i = 0; i < radiusPlus1; i++) {
            stack[cursor] = {pr, pg, pb, pa};
            cursor = next(cursor);
        }

        int yp = std::min(1, heightMinus1) * width;
        for (int i = 1; i <= radius; i++) {
            yi = (yp + x) << 2;
            int rbs = radiusPlus1 - i;
            pr = pixels[yi];
            pg = pixels[yi + 1];
            pb = pixels[yi + 2];
            pa = pixels[yi + 3];
            stack[cursor] = {pr, pg, pb, pa};
            rSum += pr * rbs;
            gSum += pg * rbs;
            bSum += pb * rbs;
            aSum += pa * rbs;
            rIn += pr;
            gIn += pg;
            bIn += pb;
            aIn += pa;
            cursor = next(cursor);
            if (i < heightMinus1) yp += width;
        }

        yi = x;
        int stackIn = 0;
        int stackOut = stackEnd;

        for (int y = 0; y < height; y++) {
            int p = yi << 2;
            double alpha = std::floor(aSum * normalization);
            pixels[p + 3] = clamped(alpha);

            if (alpha > 0) {
                double factor = 255 / alpha;
                pixels[p] = clamped(std::floor(rSum * normalization) * factor);
                pixels[p + 1] = clamped(std::floor(gSum * normalization) * factor);
                pixels[p + 2] = clamped(std::floor(bSum * normalization) * factor);
            } else {
                pixels[p] = pixels[p + 1] = pixels[p + 2] = 0;
            }

            rSum -= rOut;
            gSum -= gOut;
            bSum -= bOut;
            aSum -= aOut;

            rOut -= stack[stackIn].r;
            gOut -= stack[stackIn].g;
            bOut -= stack[stackIn].b;
            aOut -= stack[stackIn].a;

            int offset = y + radiusPlus1;
            p = (x + (offset < heightMinus1 ? offset : heightMinus1) * width) << 2;
            stack[stackIn] = {static_cast<double>(pixels[p]), static_cast<double>(pixels[p + 1]),
                              static_cast<double>(pixels[p + 2]), static_cast<double>(pixels[p + 3])};
            rIn += stack[stackIn].r;
            gIn += stack[stackIn].g;
            bIn += stack[stackIn].b;
            aIn += stack[stackIn].a;
            rSum += rIn;
            gSum += gIn;
            bSum += bIn;
            aSum += aIn;

            stackIn = next(stackIn);

            pr = stack[stackOut].r;
            pg = stack[stackOut].g;
            pb = stack[stackOut].b;
            pa = stack[stackOut].a;
            rOut += pr;
            gOut += pg;
            bOut += pb;
            aOut += pa;
            rIn -= pr;
            gIn -= pg;
            bIn -= pb;
            aIn -= pa;

            stackOut = next(stackOut);
            yi += width;
        }
    }
}

void pixelate(uint8_t* pixels, int width, int height, int pixelSize) {
    if (pixelSize <= 0) return;

    int binsX = (width + pixelSize - 1) / pixelSize;
    int binsY = (height + pixelSize - 1) / pixelSize;

    for (int binX = 0; binX < binsX; binX++) {
        for (int binY = 0; binY < binsY; binY++) {
            double red = 0, green = 0, blue = 0, alpha = 0;
            int startX = binX * pixelSize;
            int endX = std::min(startX + pixelSize, width);
            int startY = binY * pixelSize;
            int endY = std::min(startY + pixelSize, height);

            for (int x = startX; x < endX; x++) {
                for (int y = startY; y < endY; y++) {
                    size_t index = (static_cast<size_t>(width) * y + x) * 4;
                    double a = pixels[index + 3];
                    red += pixels[index] * a;
                    green += pixels[index + 1] * a;
                    blue += pixels[index + 2] * a;
                    alpha += a;
                }
            }

            if (alpha) {
                red /= alpha;
                green /= alpha;
                blue /= alpha;
            }
            alpha /= static_cast<double>(endX - startX) * (endY - startY);

            for (int x = startX; x < endX; x++) {
                for (int y = startY; y < endY; y++) {
                    size_t index = (static_cast<size_t>(width) * y + x) * 4;
                    pixels[index] = clamped(red);
                    pixels[index + 1] = clamped(green);
                    pixels[index + 2] = clamped(blue);
                    pixels[index + 3] = clamped(alpha);
                }
            }
        }
    }
}

}
