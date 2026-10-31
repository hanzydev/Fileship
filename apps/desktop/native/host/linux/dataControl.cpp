#include "clipboard.h"
#include "eventThread.h"

#include <wayland-client.h>
#include <wayland/ext-data-control-v1-client.h>
#include <wayland/wlr-data-control-unstable-v1-client.h>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <map>

namespace fileship::host {

namespace {

struct ExtProtocol {
    using Manager = ext_data_control_manager_v1;
    using Device = ext_data_control_device_v1;
    using Source = ext_data_control_source_v1;
    using Offer = ext_data_control_offer_v1;
    using DeviceListener = ext_data_control_device_v1_listener;
    using SourceListener = ext_data_control_source_v1_listener;
    using OfferListener = ext_data_control_offer_v1_listener;

    static Device* getDevice(Manager* manager, wl_seat* seat) {
        return ext_data_control_manager_v1_get_data_device(manager, seat);
    }

    static Source* createSource(Manager* manager) {
        return ext_data_control_manager_v1_create_data_source(manager);
    }

    static void listen(Device* device, DeviceListener const* listener, void* data) {
        ext_data_control_device_v1_add_listener(device, listener, data);
    }

    static void listen(Source* source, SourceListener const* listener, void* data) {
        ext_data_control_source_v1_add_listener(source, listener, data);
    }

    static void listen(Offer* offer, OfferListener const* listener, void* data) {
        ext_data_control_offer_v1_add_listener(offer, listener, data);
    }

    static void offer(Source* source, char const* type) {
        ext_data_control_source_v1_offer(source, type);
    }

    static void setSelection(Device* device, Source* source) {
        ext_data_control_device_v1_set_selection(device, source);
    }

    static void receive(Offer* offer, char const* type, int fd) {
        ext_data_control_offer_v1_receive(offer, type, fd);
    }

    static void destroy(Source* source) {
        ext_data_control_source_v1_destroy(source);
    }

    static void destroy(Offer* offer) {
        ext_data_control_offer_v1_destroy(offer);
    }
};

struct WlrProtocol {
    using Manager = zwlr_data_control_manager_v1;
    using Device = zwlr_data_control_device_v1;
    using Source = zwlr_data_control_source_v1;
    using Offer = zwlr_data_control_offer_v1;
    using DeviceListener = zwlr_data_control_device_v1_listener;
    using SourceListener = zwlr_data_control_source_v1_listener;
    using OfferListener = zwlr_data_control_offer_v1_listener;

    static Device* getDevice(Manager* manager, wl_seat* seat) {
        return zwlr_data_control_manager_v1_get_data_device(manager, seat);
    }

    static Source* createSource(Manager* manager) {
        return zwlr_data_control_manager_v1_create_data_source(manager);
    }

    static void listen(Device* device, DeviceListener const* listener, void* data) {
        zwlr_data_control_device_v1_add_listener(device, listener, data);
    }

    static void listen(Source* source, SourceListener const* listener, void* data) {
        zwlr_data_control_source_v1_add_listener(source, listener, data);
    }

    static void listen(Offer* offer, OfferListener const* listener, void* data) {
        zwlr_data_control_offer_v1_add_listener(offer, listener, data);
    }

    static void offer(Source* source, char const* type) {
        zwlr_data_control_source_v1_offer(source, type);
    }

    static void setSelection(Device* device, Source* source) {
        zwlr_data_control_device_v1_set_selection(device, source);
    }

    static void receive(Offer* offer, char const* type, int fd) {
        zwlr_data_control_offer_v1_receive(offer, type, fd);
    }

    static void destroy(Source* source) {
        zwlr_data_control_source_v1_destroy(source);
    }

    static void destroy(Offer* offer) {
        zwlr_data_control_offer_v1_destroy(offer);
    }
};

template <typename Protocol>
class DataControl final : public ClipboardBackend {
public:
    using Manager = typename Protocol::Manager;
    using Device = typename Protocol::Device;
    using Source = typename Protocol::Source;
    using Offer = typename Protocol::Offer;

    DataControl(wl_display* display, wl_seat* seat, Manager* manager) : display(display), manager(manager) {
        device = Protocol::getDevice(manager, seat);
        Protocol::listen(device, &deviceListener, this);
        wl_display_roundtrip(display);

        thread.start({
            .fd = wl_display_get_fd(display),
            .prepare = [this] { prepare(); },
            .afterPoll = [this](bool isReadable) { afterPoll(isReadable); },
        });
    }

    bool write(std::vector<std::string> const& types, ClipboardData data) override {
        return thread.run<bool>([&] {
            if (isBroken) return false;

            auto source = Protocol::createSource(manager);
            Protocol::listen(source, &sourceListener, this);
            for (auto const& type : types) Protocol::offer(source, type.c_str());
            Protocol::setSelection(device, source);
            wl_display_flush(display);

            sources[source] = {types, data};
            ownSource = source;
            return true;
        });
    }

    std::vector<std::string> types() override {
        return thread.run<std::vector<std::string>>([&]() -> std::vector<std::string> {
            if (ownSource) return sources[ownSource].first;
            if (!selection) return {};
            return offers[selection];
        });
    }

    std::optional<std::string> read(std::string const& type) override {
        struct Receive {
            ClipboardData own;
            int fd = -1;
        };

        auto receive = thread.run<Receive>([&]() -> Receive {
            if (ownSource) {
                auto const& [types, data] = sources[ownSource];
                if (std::find(types.begin(), types.end(), type) == types.end()) return {};
                return {data};
            }

            if (isBroken || !selection) return {};

            auto const& types = offers[selection];
            if (std::find(types.begin(), types.end(), type) == types.end()) return {};

            int fds[2];
            if (pipe2(fds, O_CLOEXEC) != 0) return {};

            Protocol::receive(selection, type.c_str(), fds[1]);
            wl_display_flush(display);
            close(fds[1]);
            return {nullptr, fds[0]};
        });

        if (receive.own) return *receive.own;
        if (receive.fd < 0) return std::nullopt;
        return readPipe(receive.fd);
    }

private:
    wl_display* display;
    Manager* manager;
    Device* device = nullptr;
    EventThread thread;
    bool isBroken = false;
    bool isPrepared = false;
    std::map<Offer*, std::vector<std::string>> offers;
    Offer* selection = nullptr;
    std::map<Source*, std::pair<std::vector<std::string>, ClipboardData>> sources;
    Source* ownSource = nullptr;

    void breakConnection() {
        isBroken = true;
        thread.stopWatching();
    }

    void prepare() {
        isPrepared = false;
        if (isBroken) return;

        while (wl_display_prepare_read(display) != 0) {
            if (wl_display_dispatch_pending(display) < 0) return breakConnection();
        }

        isPrepared = true;
        wl_display_flush(display);
    }

    void afterPoll(bool isReadable) {
        if (!isPrepared) return;

        if (isReadable) wl_display_read_events(display);
        else wl_display_cancel_read(display);

        if (wl_display_dispatch_pending(display) < 0) breakConnection();
    }

    void forget(Offer* offer) {
        if (!offer) return;

        Protocol::destroy(offer);
        offers.erase(offer);
    }

    static inline typename Protocol::OfferListener const offerListener{
        [](void* data, Offer* offer, char const* type) {
            static_cast<DataControl*>(data)->offers[offer].push_back(type);
        },
    };

    static inline typename Protocol::DeviceListener const deviceListener{
        [](void* data, Device*, Offer* offer) {
            auto& self = *static_cast<DataControl*>(data);
            self.offers[offer];
            Protocol::listen(offer, &offerListener, data);
        },
        [](void* data, Device*, Offer* offer) {
            auto& self = *static_cast<DataControl*>(data);
            if (self.selection != offer) self.forget(self.selection);
            self.selection = offer;
        },
        [](void* data, Device*) { static_cast<DataControl*>(data)->breakConnection(); },
        [](void* data, Device*, Offer* offer) {
            auto& self = *static_cast<DataControl*>(data);
            if (offer != self.selection) self.forget(offer);
        },
    };

    static inline typename Protocol::SourceListener const sourceListener{
        [](void* data, Source* source, char const*, int32_t fd) {
            auto& self = *static_cast<DataControl*>(data);
            auto found = self.sources.find(source);
            if (found == self.sources.end()) {
                close(fd);
                return;
            }

            writePipe(fd, found->second.second);
        },
        [](void* data, Source* source) {
            auto& self = *static_cast<DataControl*>(data);
            self.sources.erase(source);
            if (self.ownSource == source) self.ownSource = nullptr;
            Protocol::destroy(source);
        },
    };
};

struct Globals {
    wl_seat* seat = nullptr;
    ext_data_control_manager_v1* ext = nullptr;
    zwlr_data_control_manager_v1* wlr = nullptr;
};

wl_registry_listener const registryListener{
    [](void* data, wl_registry* registry, uint32_t name, char const* interface, uint32_t) {
        auto& globals = *static_cast<Globals*>(data);

        if (!std::strcmp(interface, wl_seat_interface.name) && !globals.seat) {
            globals.seat = static_cast<wl_seat*>(wl_registry_bind(registry, name, &wl_seat_interface, 1));
        } else if (!std::strcmp(interface, ext_data_control_manager_v1_interface.name)) {
            globals.ext = static_cast<ext_data_control_manager_v1*>(
                wl_registry_bind(registry, name, &ext_data_control_manager_v1_interface, 1));
        } else if (!std::strcmp(interface, zwlr_data_control_manager_v1_interface.name)) {
            globals.wlr = static_cast<zwlr_data_control_manager_v1*>(
                wl_registry_bind(registry, name, &zwlr_data_control_manager_v1_interface, 1));
        }
    },
    [](void*, wl_registry*, uint32_t) {},
};

}

std::unique_ptr<ClipboardBackend> createDataControlClipboard() {
    auto display = wl_display_connect(nullptr);
    if (!display) return nullptr;

    Globals globals;
    auto registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registryListener, &globals);
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);

    if (globals.seat && globals.ext) {
        if (globals.wlr) zwlr_data_control_manager_v1_destroy(globals.wlr);
        return std::make_unique<DataControl<ExtProtocol>>(display, globals.seat, globals.ext);
    }

    if (globals.seat && globals.wlr) return std::make_unique<DataControl<WlrProtocol>>(display, globals.seat, globals.wlr);

    if (globals.seat) wl_seat_destroy(globals.seat);
    wl_display_disconnect(display);
    return nullptr;
}

}
