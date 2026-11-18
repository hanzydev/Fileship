#include <core/core.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <fstream>
#include <stdexcept>

namespace fileship::core {

namespace {

void swapChannels(uint8_t* pixels, size_t count) {
    for (size_t index = 0; index < count; index++) std::swap(pixels[index * 4], pixels[index * 4 + 2]);
}

void freePixels(guchar* pixels, gpointer) {
    delete[] pixels;
}

Bitmap toBitmap(GdkPixbuf* source) {
    GdkPixbuf* upright = gdk_pixbuf_apply_embedded_orientation(source);
    GdkPixbuf* rgba = gdk_pixbuf_add_alpha(upright, FALSE, 0, 0, 0);

    Bitmap bitmap;
    bitmap.width = static_cast<uint32_t>(gdk_pixbuf_get_width(rgba));
    bitmap.height = static_cast<uint32_t>(gdk_pixbuf_get_height(rgba));
    bitmap.pixels.resize(static_cast<size_t>(bitmap.width) * bitmap.height * 4);

    auto stride = static_cast<size_t>(gdk_pixbuf_get_rowstride(rgba));
    auto rows = gdk_pixbuf_read_pixels(rgba);
    for (uint32_t row = 0; row < bitmap.height; row++) {
        std::copy_n(rows + row * stride, static_cast<size_t>(bitmap.width) * 4,
                    bitmap.pixels.data() + static_cast<size_t>(row) * bitmap.width * 4);
    }

    swapChannels(bitmap.pixels.data(), static_cast<size_t>(bitmap.width) * bitmap.height);

    g_object_unref(rgba);
    g_object_unref(upright);
    return bitmap;
}

}

std::vector<uint8_t> encodeImage(uint8_t const* pixels, uint32_t width, uint32_t height, EncodeOptions const& options) {
    size_t count = static_cast<size_t>(width) * height;
    auto copy = new guchar[count * 4];
    std::copy_n(pixels, count * 4, copy);
    if (options.order == PixelOrder::Bgra) swapChannels(copy, count);

    GdkPixbuf* image = gdk_pixbuf_new_from_data(copy, GDK_COLORSPACE_RGB, TRUE, 8, static_cast<int>(width),
                                                static_cast<int>(height), static_cast<int>(width) * 4, freePixels,
                                                nullptr);

    if (options.maxWidth && width > options.maxWidth) {
        int scaledHeight = std::max(1, static_cast<int>(static_cast<uint64_t>(height) * options.maxWidth / width));
        GdkPixbuf* scaled = gdk_pixbuf_scale_simple(image, static_cast<int>(options.maxWidth), scaledHeight,
                                                    GDK_INTERP_BILINEAR);
        g_object_unref(image);
        image = scaled;
    }

    gchar* buffer = nullptr;
    gsize size = 0;
    GError* error = nullptr;
    bool isSaved = false;

    if (options.format == ImageFormat::Jpeg) {
        auto quality = std::to_string(static_cast<int>(options.quality * 100));
        isSaved = gdk_pixbuf_save_to_buffer(image, &buffer, &size, "jpeg", &error, "quality", quality.c_str(), nullptr);
    } else {
        isSaved = gdk_pixbuf_save_to_buffer(image, &buffer, &size, "png", &error, nullptr);
    }

    g_object_unref(image);

    if (!isSaved) {
        std::string message = error->message;
        g_error_free(error);
        throw std::runtime_error("Could not encode the image: " + message);
    }

    std::vector<uint8_t> result(buffer, buffer + size);
    g_free(buffer);
    return result;
}

std::optional<Bitmap> decodeImage(uint8_t const* data, size_t size) {
    GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
    bool isLoaded = gdk_pixbuf_loader_write(loader, data, size, nullptr);
    isLoaded = gdk_pixbuf_loader_close(loader, nullptr) && isLoaded;

    GdkPixbuf* image = isLoaded ? gdk_pixbuf_loader_get_pixbuf(loader) : nullptr;
    std::optional<Bitmap> result;
    if (image) result = toBitmap(image);

    g_object_unref(loader);
    return result;
}

std::optional<Bitmap> decodeImageFile(std::filesystem::path const& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::nullopt;

    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return decodeImage(bytes.data(), bytes.size());
}

}
