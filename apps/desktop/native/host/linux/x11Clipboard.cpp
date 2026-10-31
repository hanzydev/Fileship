#include "clipboard.h"
#include "eventThread.h"

#include <core/linux/x11.h>

#include <X11/Xatom.h>

#include <algorithm>
#include <chrono>
#include <climits>

namespace fileship::host {

namespace {

using Clock = std::chrono::steady_clock;

constexpr auto READ_TIMEOUT = std::chrono::seconds(3);
constexpr auto TRANSFER_TIMEOUT = std::chrono::seconds(30);

class X11Clipboard final : public ClipboardBackend {
public:
    explicit X11Clipboard(Display* display) : display(display) {
        window = XCreateSimpleWindow(display, DefaultRootWindow(display), -10, -10, 1, 1, 0, 0, 0);
        XSelectInput(display, window, PropertyChangeMask);

        clipboard = atom("CLIPBOARD");
        targets = atom("TARGETS");
        timestamp = atom("TIMESTAMP");
        incremental = atom("INCR");
        property = atom("FILESHIP_SELECTION");
        clock = atom("FILESHIP_TIME");
        chunk = std::max<size_t>(4096, static_cast<size_t>(XMaxRequestSize(display)) * 4 - 1024);
        core::x11::syncErrors(display);

        thread.start({
            .fd = ConnectionNumber(display),
            .prepare = [this] { drainEvents(); },
            .afterPoll = [this](bool) { expire(); },
            .timeout = [this] { return reading || !transfers.empty() ? 500 : -1; },
        });
    }

    bool write(std::vector<std::string> const& types, ClipboardData data) override {
        return thread.run<bool>([&] {
            auto time = serverTime();
            XSetSelectionOwner(display, clipboard, window, time);
            if (XGetSelectionOwner(display, clipboard) != window) return false;

            ownTargets.clear();
            for (auto const& type : types) ownTargets.push_back(atom(type.c_str()));
            ownData = data;
            ownTime = time;
            return true;
        });
    }

    std::vector<std::string> types() override {
        auto list = convert(targets);
        if (!list) return {};

        return thread.run<std::vector<std::string>>([&] {
            std::vector<std::string> names;
            auto atoms = reinterpret_cast<long const*>(list->data());

            for (size_t index = 0; index < list->size() / sizeof(long); index++) {
                auto name = core::x11::atomName(display, static_cast<Atom>(atoms[index]));
                if (!name.empty()) names.push_back(name);
            }

            core::x11::syncErrors(display);
            return names;
        });
    }

    std::optional<std::string> read(std::string const& type) override {
        return convert(thread.run<Atom>([&] { return atom(type.c_str()); }));
    }

private:
    struct Transfer {
        Window requestor;
        Atom property;
        Atom type;
        ClipboardData data;
        size_t offset = 0;
        Clock::time_point updated;
    };

    struct Reading {
        Atom target;
        std::promise<std::optional<std::string>> result;
        std::string data;
        bool isIncremental = false;
        Clock::time_point deadline;
    };

    Display* display;
    Window window;
    Atom clipboard;
    Atom targets;
    Atom timestamp;
    Atom incremental;
    Atom property;
    Atom clock;
    size_t chunk;
    EventThread thread;
    std::mutex readMutex;
    std::vector<Atom> ownTargets;
    ClipboardData ownData;
    Time ownTime = CurrentTime;
    std::vector<Transfer> transfers;
    std::unique_ptr<Reading> reading;

    Atom atom(char const* name) {
        return core::x11::atom(display, name);
    }

    Time serverTime() {
        XChangeProperty(display, window, clock, XA_STRING, 8, PropModeAppend, nullptr, 0);

        XEvent event{};
        XIfEvent(
            display, &event,
            [](Display*, XEvent* candidate, XPointer data) -> Bool {
                auto& self = *reinterpret_cast<X11Clipboard*>(data);
                return candidate->type == PropertyNotify && candidate->xproperty.window == self.window &&
                       candidate->xproperty.atom == self.clock;
            },
            reinterpret_cast<XPointer>(this));
        return event.xproperty.time;
    }

    std::optional<std::string> convert(Atom target) {
        std::lock_guard lock(readMutex);

        auto result = thread.run<std::future<std::optional<std::string>>>([this, target] {
            auto next = std::make_unique<Reading>();
            next->target = target;
            auto future = next->result.get_future();

            if (ownData) {
                next->result.set_value(ownValue(target));
                return future;
            }

            next->deadline = Clock::now() + READ_TIMEOUT;
            reading = std::move(next);
            XDeleteProperty(display, window, property);
            XConvertSelection(display, clipboard, target, property, window, CurrentTime);
            XFlush(display);
            return future;
        });

        return result.get();
    }

    std::optional<std::string> ownValue(Atom target) {
        if (target == targets) {
            std::vector<long> list{static_cast<long>(targets), static_cast<long>(timestamp)};
            for (auto value : ownTargets) list.push_back(static_cast<long>(value));
            return std::string(reinterpret_cast<char const*>(list.data()), list.size() * sizeof(long));
        }

        if (std::find(ownTargets.begin(), ownTargets.end(), target) == ownTargets.end()) return std::nullopt;
        return *ownData;
    }

    void finishReading(std::optional<std::string> value) {
        if (!reading) return;

        reading->result.set_value(std::move(value));
        reading.reset();
    }

    std::optional<std::string> takeProperty(Atom* type) {
        int format = 0;
        unsigned long count = 0;
        unsigned long remaining = 0;
        unsigned char* data = nullptr;

        if (XGetWindowProperty(display, window, property, 0, LONG_MAX / 4, True, AnyPropertyType, type, &format,
                               &count, &remaining, &data) != Success)
            return std::nullopt;

        size_t size = format == 32 ? sizeof(long) : static_cast<size_t>(format / 8);
        std::string value(reinterpret_cast<char const*>(data), count * size);
        if (data) XFree(data);
        return value;
    }

    void drainEvents() {
        while (XPending(display)) {
            XEvent event{};
            XNextEvent(display, &event);

            if (event.type == SelectionRequest) handleRequest(event.xselectionrequest);
            else if (event.type == SelectionClear && event.xselectionclear.selection == clipboard) ownData.reset();
            else if (event.type == SelectionNotify) handleNotify(event.xselection);
            else if (event.type == PropertyNotify) handleProperty(event.xproperty);
        }
    }

    void handleRequest(XSelectionRequestEvent const& request) {
        XSelectionEvent reply{};
        reply.type = SelectionNotify;
        reply.display = display;
        reply.requestor = request.requestor;
        reply.selection = request.selection;
        reply.target = request.target;
        reply.time = request.time;
        reply.property = None;

        auto target = request.property ? request.property : request.target;
        bool isCurrent = ownData && request.selection == clipboard &&
                         (request.time == CurrentTime || request.time >= ownTime);

        if (isCurrent && request.target == timestamp) {
            long value = static_cast<long>(ownTime);
            XChangeProperty(display, request.requestor, target, XA_INTEGER, 32, PropModeReplace,
                            reinterpret_cast<unsigned char const*>(&value), 1);
            reply.property = target;
        } else if (isCurrent && request.target != None) {
            if (auto value = ownValue(request.target)) {
                if (request.target == targets) {
                    XChangeProperty(display, request.requestor, target, XA_ATOM, 32, PropModeReplace,
                                    reinterpret_cast<unsigned char const*>(value->data()),
                                    static_cast<int>(value->size() / sizeof(long)));
                } else if (ownData->size() <= chunk) {
                    XChangeProperty(display, request.requestor, target, request.target, 8, PropModeReplace,
                                    reinterpret_cast<unsigned char const*>(ownData->data()),
                                    static_cast<int>(ownData->size()));
                } else {
                    long size = static_cast<long>(ownData->size());
                    XSelectInput(display, request.requestor, PropertyChangeMask);
                    XChangeProperty(display, request.requestor, target, incremental, 32, PropModeReplace,
                                    reinterpret_cast<unsigned char const*>(&size), 1);
                    transfers.push_back({request.requestor, target, request.target, ownData, 0, Clock::now()});
                }

                reply.property = target;
            }
        }

        XSendEvent(display, request.requestor, False, NoEventMask, reinterpret_cast<XEvent*>(&reply));
        core::x11::syncErrors(display);
    }

    void handleNotify(XSelectionEvent const& notify) {
        if (!reading || notify.requestor != window || notify.selection != clipboard) return;
        if (notify.property == None) return finishReading(std::nullopt);

        Atom type = None;
        auto value = takeProperty(&type);
        if (!value) return finishReading(std::nullopt);

        if (type == incremental) {
            reading->isIncremental = true;
            reading->deadline = Clock::now() + READ_TIMEOUT;
            return;
        }

        finishReading(std::move(value));
    }

    void handleProperty(XPropertyEvent const& event) {
        if (event.window == window) {
            if (!reading || !reading->isIncremental || event.atom != property || event.state != PropertyNewValue)
                return;

            Atom type = None;
            auto value = takeProperty(&type);
            if (!value) return finishReading(std::nullopt);
            if (value->empty()) return finishReading(std::move(reading->data));

            reading->data += *value;
            reading->deadline = Clock::now() + READ_TIMEOUT;
            return;
        }

        if (event.state != PropertyDelete) return;

        auto found = std::find_if(transfers.begin(), transfers.end(), [&](Transfer const& transfer) {
            return transfer.requestor == event.window && transfer.property == event.atom;
        });
        if (found == transfers.end()) return;

        auto size = std::min(chunk, found->data->size() - found->offset);
        XChangeProperty(display, found->requestor, found->property, found->type, 8, PropModeReplace,
                        reinterpret_cast<unsigned char const*>(found->data->data() + found->offset),
                        static_cast<int>(size));
        found->offset += size;
        found->updated = Clock::now();

        if (!size) endTransfer(found);
        core::x11::syncErrors(display);
    }

    void endTransfer(std::vector<Transfer>::iterator transfer) {
        auto requestor = transfer->requestor;
        transfers.erase(transfer);

        bool isWatched = std::any_of(transfers.begin(), transfers.end(),
                                     [&](Transfer const& other) { return other.requestor == requestor; });
        if (!isWatched) XSelectInput(display, requestor, NoEventMask);
    }

    void expire() {
        auto now = Clock::now();
        if (reading && now > reading->deadline) finishReading(std::nullopt);

        auto stale = std::find_if(transfers.begin(), transfers.end(),
                                  [&](Transfer const& transfer) { return now - transfer.updated > TRANSFER_TIMEOUT; });
        if (stale == transfers.end()) return;

        endTransfer(stale);
        core::x11::syncErrors(display);
    }
};

}

std::unique_ptr<ClipboardBackend> createX11Clipboard() {
    auto display = core::x11::openDisplay();
    if (!display) return nullptr;

    return std::make_unique<X11Clipboard>(display);
}

}
