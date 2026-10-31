#include <core/core.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace fileship::core {

namespace {

struct Pixbuf {
    GdkPixbuf* value = nullptr;

    explicit Pixbuf(GdkPixbuf* pixbuf) : value(pixbuf) {}

    ~Pixbuf() {
        if (value) g_object_unref(value);
    }

    Pixbuf(Pixbuf const&) = delete;
    Pixbuf& operator=(Pixbuf const&) = delete;
};

}

std::vector<uint8_t> encodeImage(uint8_t const* pixels, uint32_t width, uint32_t height,
                                 EncodeOptions const& options) {
    bool isJpeg = options.format == ImageFormat::Jpeg;
    int channels = isJpeg ? 3 : 4;
    bool isBgra = options.order == PixelOrder::Bgra;

    std::vector<uint8_t> converted(static_cast<size_t>(width) * height * channels);
    for (size_t index = 0, count = static_cast<size_t>(width) * height; index < count; index++) {
        auto source = pixels + index * 4;
        auto target = converted.data() + index * channels;
        target[0] = source[isBgra ? 2 : 0];
        target[1] = source[1];
        target[2] = source[isBgra ? 0 : 2];
        if (!isJpeg) target[3] = source[3];
    }

    Pixbuf pixbuf(gdk_pixbuf_new_from_data(converted.data(), GDK_COLORSPACE_RGB, !isJpeg, 8, static_cast<int>(width),
                                           static_cast<int>(height), static_cast<int>(width) * channels, nullptr,
                                           nullptr));
    if (!pixbuf.value) throw std::runtime_error("Could not encode the image: source");

    GdkPixbuf* source = pixbuf.value;
    Pixbuf scaled(nullptr);

    if (options.maxWidth && width > options.maxWidth) {
        auto targetHeight = std::max<uint32_t>(
            1, static_cast<uint32_t>(std::lround(static_cast<double>(height) * options.maxWidth / width)));
        scaled.value = gdk_pixbuf_scale_simple(source, static_cast<int>(options.maxWidth),
                                               static_cast<int>(targetHeight), GDK_INTERP_HYPER);
        if (!scaled.value) throw std::runtime_error("Could not encode the image: scale");
        source = scaled.value;
    }

    gchar* buffer = nullptr;
    gsize size = 0;
    GError* error = nullptr;
    auto quality = std::to_string(std::clamp(static_cast<int>(std::lround(options.quality * 100)), 1, 100));

    bool saved = isJpeg ? gdk_pixbuf_save_to_buffer(source, &buffer, &size, "jpeg", &error, "quality",
                                                    quality.c_str(), nullptr)
                        : gdk_pixbuf_save_to_buffer(source, &buffer, &size, "png", &error, nullptr);

    if (!saved) {
        std::string message = error ? error->message : "unknown error";
        if (error) g_error_free(error);
        throw std::runtime_error("Could not encode the image: " + message);
    }

    std::vector<uint8_t> output(reinterpret_cast<uint8_t*>(buffer), reinterpret_cast<uint8_t*>(buffer) + size);
    g_free(buffer);
    return output;
}

std::optional<Bitmap> decodeImage(uint8_t const* data, size_t size) {
    auto loader = gdk_pixbuf_loader_new();
    bool loaded = gdk_pixbuf_loader_write(loader, data, size, nullptr) && gdk_pixbuf_loader_close(loader, nullptr);
    auto pixbuf = loaded ? gdk_pixbuf_loader_get_pixbuf(loader) : nullptr;

    if (!pixbuf) {
        if (!loaded) gdk_pixbuf_loader_close(loader, nullptr);
        g_object_unref(loader);
        return std::nullopt;
    }

    Bitmap bitmap;
    bitmap.width = static_cast<uint32_t>(gdk_pixbuf_get_width(pixbuf));
    bitmap.height = static_cast<uint32_t>(gdk_pixbuf_get_height(pixbuf));
    bitmap.pixels.resize(static_cast<size_t>(bitmap.width) * bitmap.height * 4);

    int channels = gdk_pixbuf_get_n_channels(pixbuf);
    int stride = gdk_pixbuf_get_rowstride(pixbuf);
    auto pixels = gdk_pixbuf_read_pixels(pixbuf);

    for (uint32_t y = 0; y < bitmap.height; y++) {
        for (uint32_t x = 0; x < bitmap.width; x++) {
            auto source = pixels + static_cast<size_t>(y) * stride + static_cast<size_t>(x) * channels;
            auto target = bitmap.pixels.data() + (static_cast<size_t>(y) * bitmap.width + x) * 4;
            target[0] = source[2];
            target[1] = source[1];
            target[2] = source[0];
            target[3] = channels == 4 ? source[3] : 255;
        }
    }

    g_object_unref(loader);
    return bitmap;
}

std::optional<Bitmap> decodeImageFile(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::nullopt;

    std::vector<uint8_t> bytes(std::istreambuf_iterator<char>(file), {});
    return decodeImage(bytes.data(), bytes.size());
}

}
