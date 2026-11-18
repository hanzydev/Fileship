#include "host.h"

#include <shared/log.h>

#include <xcb/xcb.h>

#include <poll.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <map>
#include <mutex>
#include <thread>

namespace fileship::host {

namespace {

constexpr size_t CHUNK_SIZE = 128 * 1024;
constexpr int64_t READ_TIMEOUT_MS = 2000;
constexpr int64_t HISTORY_GAP_MS = 1000;

using Bytes = std::shared_ptr<std::vector<uint8_t> const>;
using Event = std::unique_ptr<xcb_generic_event_t, decltype(&free)>;

std::atomic<int64_t> lastImageWriteAt = 0;

xcb_atom_t atom(xcb_connection_t* connection, std::string const& name) {
    auto reply = xcb_intern_atom_reply(
        connection, xcb_intern_atom(connection, 0, static_cast<uint16_t>(name.size()), name.c_str()), nullptr);
    auto value = reply->atom;
    free(reply);
    return value;
}

xcb_window_t createWindow(xcb_connection_t* connection) {
    auto screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
    auto window = xcb_generate_id(connection);
    uint32_t events = XCB_EVENT_MASK_PROPERTY_CHANGE;

    xcb_create_window(connection, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, 1, 1, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, XCB_CW_EVENT_MASK, &events);
    return window;
}

class Owner {
public:
    explicit Owner(xcb_connection_t* connection)
        : connection(connection), window(createWindow(connection)), clipboard(atom(connection, "CLIPBOARD")),
          targets(atom(connection, "TARGETS")), incr(atom(connection, "INCR")) {
        std::thread([this] { run(); }).detach();
    }

    bool offer(std::vector<std::pair<std::string, Bytes>> const& values) {
        std::lock_guard lock(mutex);

        offers.clear();
        for (auto const& [target, data] : values) offers.push_back({atom(connection, target), data});

        xcb_set_selection_owner(connection, window, clipboard, XCB_CURRENT_TIME);
        return isOwner();
    }

private:
    struct Offer {
        xcb_atom_t target;
        Bytes data;
    };

    struct Transfer {
        xcb_atom_t target;
        Bytes data;
        size_t offset = 0;
    };

    bool isOwner() {
        auto reply = xcb_get_selection_owner_reply(connection, xcb_get_selection_owner(connection, clipboard), nullptr);
        bool isOurs = reply && reply->owner == window;
        free(reply);
        return isOurs;
    }

    void write(xcb_window_t requestor, xcb_atom_t property, xcb_atom_t type, uint8_t format, size_t count,
               void const* data) {
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, requestor, property, type, format,
                            static_cast<uint32_t>(count), data);
    }

    bool answer(xcb_selection_request_event_t const& request) {
        if (request.target == targets) {
            std::vector<xcb_atom_t> atoms{targets};
            for (auto const& offer : offers) atoms.push_back(offer.target);

            write(request.requestor, request.property, XCB_ATOM_ATOM, 32, atoms.size(), atoms.data());
            return true;
        }

        for (auto const& offer : offers) {
            if (offer.target != request.target) continue;

            if (offer.data->size() <= CHUNK_SIZE) {
                write(request.requestor, request.property, offer.target, 8, offer.data->size(), offer.data->data());
                return true;
            }

            uint32_t events = XCB_EVENT_MASK_PROPERTY_CHANGE;
            auto size = static_cast<uint32_t>(offer.data->size());

            xcb_change_window_attributes(connection, request.requestor, XCB_CW_EVENT_MASK, &events);
            write(request.requestor, request.property, incr, 32, 1, &size);
            transfers[{request.requestor, request.property}] = {offer.target, offer.data, 0};
            return true;
        }

        return false;
    }

    void handleRequest(xcb_selection_request_event_t const& request) {
        xcb_selection_notify_event_t notify{};
        notify.response_type = XCB_SELECTION_NOTIFY;
        notify.time = request.time;
        notify.requestor = request.requestor;
        notify.selection = request.selection;
        notify.target = request.target;
        notify.property = answer(request) ? request.property : static_cast<xcb_atom_t>(XCB_NONE);

        xcb_send_event(connection, 0, request.requestor, XCB_EVENT_MASK_NO_EVENT,
                       reinterpret_cast<char const*>(&notify));
    }

    void handleProperty(xcb_property_notify_event_t const& event) {
        if (event.state != XCB_PROPERTY_DELETE) return;

        auto found = transfers.find({event.window, event.atom});
        if (found == transfers.end()) return;

        auto& transfer = found->second;
        auto size = std::min(CHUNK_SIZE, transfer.data->size() - transfer.offset);

        write(event.window, event.atom, transfer.target, 8, size, transfer.data->data() + transfer.offset);
        transfer.offset += size;
        if (!size) transfers.erase(found);
    }

    void run() {
        while (auto event = xcb_wait_for_event(connection)) {
            {
                std::lock_guard lock(mutex);
                auto type = event->response_type & 0x7f;

                if (type == XCB_SELECTION_REQUEST) {
                    handleRequest(*reinterpret_cast<xcb_selection_request_event_t*>(event));
                } else if (type == XCB_PROPERTY_NOTIFY) {
                    handleProperty(*reinterpret_cast<xcb_property_notify_event_t*>(event));
                } else if (type == XCB_SELECTION_CLEAR && !isOwner()) {
                    offers.clear();
                }
            }

            free(event);
            xcb_flush(connection);
        }
    }

    xcb_connection_t* connection;
    xcb_window_t window;
    xcb_atom_t clipboard;
    xcb_atom_t targets;
    xcb_atom_t incr;
    std::mutex mutex;
    std::vector<Offer> offers;
    std::map<std::pair<xcb_window_t, xcb_atom_t>, Transfer> transfers;
};

Owner* owner() {
    static Owner* const instance = []() -> Owner* {
        auto connection = xcb_connect(nullptr, nullptr);
        if (!xcb_connection_has_error(connection)) return new Owner(connection);

        logWarn(L"There is no X server, nothing can be copied to the clipboard");
        return nullptr;
    }();

    return instance;
}

bool copy(wchar_t const* label, std::vector<std::pair<std::string, Bytes>> const& values) {
    if (owner() && owner()->offer(values)) return true;

    logWarn(std::wstring(L"Could not copy the ") + label);
    return false;
}

class Reader {
public:
    Reader() : connection(xcb_connect(nullptr, nullptr)) {
        if (xcb_connection_has_error(connection)) return;

        window = createWindow(connection);
        clipboard = atom(connection, "CLIPBOARD");
        property = atom(connection, "FILESHIP_CLIPBOARD");
        incr = atom(connection, "INCR");

        if (auto list = read("TARGETS")) {
            auto atoms = reinterpret_cast<xcb_atom_t const*>(list->data());
            targets.assign(atoms, atoms + list->size() / sizeof(xcb_atom_t));
        }
    }

    ~Reader() {
        xcb_disconnect(connection);
    }

    bool has(char const* target) {
        return !targets.empty() && std::find(targets.begin(), targets.end(), atom(connection, target)) != targets.end();
    }

    std::optional<std::vector<uint8_t>> read(char const* target) {
        if (xcb_connection_has_error(connection)) return std::nullopt;

        xcb_convert_selection(connection, window, clipboard, atom(connection, target), property, XCB_CURRENT_TIME);
        xcb_flush(connection);

        auto notify = next(XCB_SELECTION_NOTIFY);
        if (!notify || reinterpret_cast<xcb_selection_notify_event_t*>(notify.get())->property == XCB_NONE) {
            return std::nullopt;
        }

        auto [type, data] = takeProperty();
        if (type != incr) return data;

        std::vector<uint8_t> result;

        while (true) {
            auto event = next(XCB_PROPERTY_NOTIFY);
            if (!event) return std::nullopt;
            if (reinterpret_cast<xcb_property_notify_event_t*>(event.get())->state != XCB_PROPERTY_NEW_VALUE) continue;

            auto chunk = takeProperty().second;
            if (chunk.empty()) return result;

            result.insert(result.end(), chunk.begin(), chunk.end());
        }
    }

private:
    Event next(int type) {
        auto deadline = nowMs() + READ_TIMEOUT_MS;

        while (true) {
            while (auto event = xcb_poll_for_event(connection)) {
                Event owned(event, free);
                if ((event->response_type & 0x7f) == type) return owned;
            }

            auto remaining = deadline - nowMs();
            pollfd descriptor{xcb_get_file_descriptor(connection), POLLIN, 0};
            if (remaining <= 0 || poll(&descriptor, 1, static_cast<int>(remaining)) <= 0) return Event(nullptr, free);
        }
    }

    std::pair<xcb_atom_t, std::vector<uint8_t>> takeProperty() {
        auto reply = xcb_get_property_reply(
            connection, xcb_get_property(connection, 1, window, property, XCB_GET_PROPERTY_TYPE_ANY, 0, UINT32_MAX),
            nullptr);
        if (!reply) return {};

        auto bytes = static_cast<uint8_t const*>(xcb_get_property_value(reply));
        std::pair<xcb_atom_t, std::vector<uint8_t>> result{
            reply->type, std::vector<uint8_t>(bytes, bytes + xcb_get_property_value_length(reply))};

        free(reply);
        xcb_flush(connection);
        return result;
    }

    xcb_connection_t* connection;
    xcb_window_t window = 0;
    xcb_atom_t clipboard = 0;
    xcb_atom_t property = 0;
    xcb_atom_t incr = 0;
    std::vector<xcb_atom_t> targets;
};

}

void writeClipboardText(std::wstring const& text) {
    auto wait = lastImageWriteAt + HISTORY_GAP_MS - nowMs();
    if (wait > 0) std::this_thread::sleep_for(std::chrono::milliseconds(wait));

    auto utf8 = toUtf8(text);
    auto data = std::make_shared<std::vector<uint8_t> const>(utf8.begin(), utf8.end());
    copy(L"text", {{"UTF8_STRING", data}, {"text/plain;charset=utf-8", data}, {"text/plain", data}});
}

bool writeClipboardImage(std::vector<uint8_t> const& png, core::Bitmap const&) {
    bool isCopied = copy(L"image", {{"image/png", std::make_shared<std::vector<uint8_t> const>(png)}});
    if (isCopied) lastImageWriteAt = nowMs();
    return isCopied;
}

std::wstring readClipboardText() {
    Reader reader;
    auto data = reader.has("UTF8_STRING") ? reader.read("UTF8_STRING") : std::nullopt;
    return data ? toWide(std::string(data->begin(), data->end())) : std::wstring();
}

std::optional<core::Bitmap> readClipboardImage() {
    Reader reader;

    for (auto target : {"image/png", "image/jpeg", "image/bmp"}) {
        if (!reader.has(target)) continue;
        if (auto data = reader.read(target)) return core::decodeImage(data->data(), data->size());
    }

    return std::nullopt;
}

std::optional<std::filesystem::path> readClipboardFilePath() {
    Reader reader;
    auto data = reader.has("text/uri-list") ? reader.read("text/uri-list") : std::nullopt;
    if (!data) return std::nullopt;

    std::string list(data->begin(), data->end());
    std::string first = list.substr(0, list.find_first_of("\r\n"));

    gchar* path = g_filename_from_uri(first.c_str(), nullptr, nullptr);
    if (!path) return std::nullopt;

    std::filesystem::path result = path;
    g_free(path);
    return result;
}

}
