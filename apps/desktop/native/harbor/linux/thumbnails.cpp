#include "common.h"

#include <shared/http.h>
#include <shared/utf8.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>

namespace harbor {

namespace {

struct Size {
    char const* folder;
    int pixels;
};

constexpr std::array<Size, 3> SIZES = {{{"normal", 128}, {"large", 256}, {"x-large", 512}}};
constexpr std::array<char const*, 16> PREVIEWED = {".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".avif", ".heic",
                                                   ".tif", ".tiff", ".mp4", ".webm", ".mkv", ".mov", ".avi", ".pdf"};

std::mutex mutex;
std::condition_variable wake;
std::deque<std::string> waiting;
std::thread worker;
bool isStopping = false;

bool isPreviewed(std::string const& relative) {
    std::string lower(relative);
    std::ranges::transform(lower, lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return std::ranges::any_of(PREVIEWED, [&](char const* ending) { return lower.ends_with(ending); });
}

GdkPixbuf* load(std::vector<char> const& bytes) {
    GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
    bool isRead = gdk_pixbuf_loader_write(loader, reinterpret_cast<guchar const*>(bytes.data()), bytes.size(), nullptr);
    bool isClosed = gdk_pixbuf_loader_close(loader, nullptr);
    GdkPixbuf* image = isRead && isClosed ? gdk_pixbuf_loader_get_pixbuf(loader) : nullptr;

    if (image) g_object_ref(image);
    g_object_unref(loader);
    return image;
}

void save(GdkPixbuf* image, Size const& size, char const* name, char const* uri, struct stat const& info) {
    int width = gdk_pixbuf_get_width(image);
    int height = gdk_pixbuf_get_height(image);
    double scale = std::min(1.0, static_cast<double>(size.pixels) / std::max(width, height));
    GdkPixbuf* scaled = gdk_pixbuf_scale_simple(image, std::max(1, static_cast<int>(width * scale + 0.5)),
                                                std::max(1, static_cast<int>(height * scale + 0.5)), GDK_INTERP_BILINEAR);

    auto folder = std::filesystem::path(g_get_user_cache_dir()) / "thumbnails" / size.folder;
    auto file = folder / (std::string(name) + ".png");
    auto draft = folder / (std::string(name) + "." + std::to_string(getpid()) + ".tmp");
    std::error_code error;
    std::filesystem::create_directories(folder, error);

    bool isSaved = gdk_pixbuf_save(scaled, draft.c_str(), "png", nullptr, "tEXt::Thumb::URI", uri, "tEXt::Thumb::MTime",
                                   std::to_string(info.st_mtim.tv_sec).c_str(), "tEXt::Thumb::Size",
                                   std::to_string(info.st_size).c_str(), "tEXt::Software", "Fileship", nullptr);
    g_object_unref(scaled);

    if (isSaved) {
        chmod(draft.c_str(), 0600);
        std::filesystem::rename(draft, file, error);
    }
}

void render(std::string const& relative) {
    auto entry = entryOf(relative);
    if (!entry.exists || entry.isFolder || !entry.remote || !entry.identity || !entry.identity->starts_with("f:")) return;

    auto path = L"/u/" + fileship::toWide(entry.identity->substr(2)) + L"/thumbnail";
    auto response = fileship::httpSend(endpoint(), {L"GET", path, {}, {}, 30000, {}});
    if (response.status != 200 || response.body.empty()) return;

    GdkPixbuf* image = load(response.body);
    if (!image) return;

    struct stat info {};
    lstat(backingOf(relative).c_str(), &info);

    gchar* uri = g_filename_to_uri((config().root + "/" + relative).c_str(), nullptr, nullptr);
    gchar* name = g_compute_checksum_for_string(G_CHECKSUM_MD5, uri, -1);

    for (auto const& size : SIZES) save(image, size, name, uri, info);

    g_free(name);
    g_free(uri);
    g_object_unref(image);
}

void run() {
    std::unique_lock lock(mutex);

    while (true) {
        wake.wait(lock, [] { return isStopping || !waiting.empty(); });
        if (isStopping) return;

        auto relative = std::move(waiting.front());
        waiting.pop_front();
        lock.unlock();

        try {
            render(relative);
        } catch (std::exception const& error) {
            log("Could not prepare the thumbnail of " + relative + ": " + error.what());
        }

        lock.lock();
    }
}

}

void queueThumbnail(std::string const& relative) {
    if (!isPreviewed(relative)) return;

    std::lock_guard lock(mutex);
    waiting.push_back(relative);
    if (!worker.joinable()) worker = std::thread(run);
    wake.notify_one();
}

void stopThumbnails() {
    {
        std::lock_guard lock(mutex);
        isStopping = true;
        waiting.clear();
        wake.notify_one();
    }

    if (worker.joinable()) worker.join();
    isStopping = false;
}

}
