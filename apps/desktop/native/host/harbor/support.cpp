#include "support.h"

#include <shared/hash.h>
#include <shared/i18n.h>
#include <shared/log.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <chrono>
#include <ctime>
#include <sstream>

namespace fileship::host::harbor {

SerialQueue::SerialQueue(std::wstring name) : name(std::move(name)) {
    std::promise<void> ready;
    ready.set_value();
    last = ready.get_future().share();
    worker = std::thread([this] { run(); });
}

SerialQueue::~SerialQueue() {
    post({});
    worker.join();
}

std::shared_future<void> SerialQueue::post(std::function<void()> task) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future = promise->get_future().share();

    {
        std::lock_guard lock(mutex);
        tasks.emplace_back(std::move(task), promise);
        last = future;
    }

    wake.notify_one();
    return future;
}

void SerialQueue::drain() {
    std::shared_future<void> current;

    {
        std::lock_guard lock(mutex);
        current = last;
    }

    current.wait();
}

void SerialQueue::run() {
    while (true) {
        std::function<void()> task;
        std::shared_ptr<std::promise<void>> promise;

        {
            std::unique_lock lock(mutex);
            wake.wait(lock, [this] { return !tasks.empty(); });
            task = std::move(tasks.front().first);
            promise = std::move(tasks.front().second);
            tasks.pop_front();
        }

        if (!task) {
            promise->set_value();
            return;
        }

        try {
            task();
            promise->set_value();
        } catch (std::exception const& error) {
            logError(name + L" task failed: " + errorMessage(error));
            promise->set_exception(std::current_exception());
        } catch (...) {
            logError(name + L" task failed: " + describeException(std::current_exception()));
            promise->set_exception(std::current_exception());
        }
    }
}

Timers::Timers() : worker([this] { run(); }) {}

Timers::~Timers() {
    {
        std::lock_guard lock(mutex);
        stopping = true;
    }

    wake.notify_all();
    worker.join();
}

uint64_t Timers::set(int delayMs, std::function<void()> callback) {
    uint64_t id = 0;

    {
        std::lock_guard lock(mutex);
        id = nextId++;
        pending.emplace(Clock::now() + std::chrono::milliseconds(delayMs),
                        std::make_pair(id, std::move(callback)));
    }

    wake.notify_all();
    return id;
}

void Timers::clear(uint64_t id) {
    if (!id) return;

    std::lock_guard lock(mutex);
    for (auto item = pending.begin(); item != pending.end(); ++item) {
        if (item->second.first == id) {
            pending.erase(item);
            return;
        }
    }
}

void Timers::run() {
    std::unique_lock lock(mutex);

    while (!stopping) {
        if (pending.empty()) {
            wake.wait(lock);
            continue;
        }

        auto first = pending.begin();
        if (first->first > Clock::now()) {
            wake.wait_until(lock, first->first);
            continue;
        }

        auto callback = std::move(first->second.second);
        pending.erase(first);
        lock.unlock();

        try {
            callback();
        } catch (std::exception const& error) {
            logError(L"Harbor timer failed: " + errorMessage(error));
        } catch (...) {
            logError(L"Harbor timer failed: " + describeException(std::current_exception()));
        }

        lock.lock();
    }
}

Timers& timers() {
    static auto* instance = new Timers();
    return *instance;
}

int64_t nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int64_t parseIsoDate(std::wstring const& text) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    double second = 0;
    wchar_t separator = 0;

    std::wistringstream stream(text);
    stream >> year >> separator >> month >> separator >> day >> separator >> hour >> separator >> minute >> separator >>
        second;
    if (!stream && !stream.eof()) return 0;

    std::wstring zone;
    std::getline(stream, zone);

    using namespace std::chrono;
    auto days = sys_days(std::chrono::year(year) / month / day).time_since_epoch().count();
    int64_t result = ((static_cast<int64_t>(days) * 24 + hour) * 60 + minute) * 60000 +
                     static_cast<int64_t>(std::llround(second * 1000));

    if (zone.size() >= 6 && (zone[0] == L'+' || zone[0] == L'-')) {
        int offset = std::stoi(zone.substr(1, 2)) * 60 + std::stoi(zone.substr(4, 2));
        result -= (zone[0] == L'+' ? 1 : -1) * static_cast<int64_t>(offset) * 60000;
    }

    return result;
}

std::wstring randomHex(size_t bytes) {
    std::vector<uint8_t> random(bytes);
    randomBytes(random.data(), random.size());
    return toWide(toHex(random));
}

std::string sha1Hex(std::string const& data) {
    return hashHex(HashKind::Sha1, data);
}

std::string sha256Hex(std::string const& data) {
    return hashHex(HashKind::Sha256, data);
}

bool equalsIgnoreCase(std::wstring const& a, std::wstring const& b) {
    return a.size() == b.size() && lower(a) == lower(b);
}

bool startsWith(std::wstring const& text, std::wstring const& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

std::wstring dirnameOf(std::wstring const& path) {
    auto slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return L".";
    if (slash == 0) return path.substr(0, 1);
    return path.substr(0, slash);
}

std::wstring basenameOf(std::wstring const& path) {
    auto slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

std::wstring extnameOf(std::wstring const& path) {
    auto name = basenameOf(path);
    auto dot = name.find_last_of(L'.');
    if (dot == std::wstring::npos || dot == 0) return {};
    return name.substr(dot);
}

std::wstring formatSize(int64_t bytes) {
    static constexpr std::array<wchar_t const*, 9> SYMBOLS = {L"B",  L"KB", L"MB", L"GB", L"TB",
                                                              L"PB", L"EB", L"ZB", L"YB"};

    int exponent = bytes > 0 ? static_cast<int>(std::floor(std::log(static_cast<double>(bytes)) /
                                                            std::log(1024.0)))
                             : 0;
    exponent = std::clamp(exponent, 0, 8);

    double value = static_cast<double>(bytes) / std::pow(1024.0, exponent);
    value = std::round(value * 100) / 100;

    if (value == 1024 && exponent < 8) {
        value = 1;
        exponent++;
    }

    return formatNumber(value, 3) + L" " + SYMBOLS[exponent];
}

std::wstring errorMessage(std::exception const& error) {
    return toWide(error.what());
}

std::wstring mimeTypeOf(std::wstring const& path) {
    static std::map<std::wstring, std::wstring> const TYPES = {
        {L".png", L"image/png"},        {L".jpg", L"image/jpeg"},
        {L".jpeg", L"image/jpeg"},      {L".gif", L"image/gif"},
        {L".webp", L"image/webp"},      {L".bmp", L"image/bmp"},
        {L".svg", L"image/svg+xml"},    {L".mp4", L"video/mp4"},
        {L".webm", L"video/webm"},      {L".mkv", L"video/x-matroska"},
        {L".mov", L"video/quicktime"},  {L".mp3", L"audio/mpeg"},
        {L".wav", L"audio/wav"},        {L".ogg", L"audio/ogg"},
        {L".pdf", L"application/pdf"},  {L".zip", L"application/zip"},
        {L".txt", L"text/plain"},       {L".md", L"text/markdown"},
        {L".json", L"application/json"}, {L".html", L"text/html"},
        {L".css", L"text/css"},         {L".js", L"text/javascript"},
        {L".ts", L"text/plain"},
    };

    auto found = TYPES.find(lower(extnameOf(path)));
    return found == TYPES.end() ? L"application/octet-stream" : found->second;
}

std::optional<std::wstring> stringField(JsonObject const& object, std::wstring const& key) {
    if (!object.HasKey(key)) return std::nullopt;

    auto value = object.GetNamedValue(key);
    if (value.ValueType() != JsonValueType::String) return std::nullopt;
    return std::wstring(value.GetString());
}

std::optional<double> numberField(JsonObject const& object, std::wstring const& key) {
    if (!object.HasKey(key)) return std::nullopt;

    auto value = object.GetNamedValue(key);
    if (value.ValueType() == JsonValueType::Number) return value.GetNumber();
    if (value.ValueType() == JsonValueType::String) {
        try {
            return std::stod(std::wstring(value.GetString()));
        } catch (std::exception const&) {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

std::optional<bool> boolField(JsonObject const& object, std::wstring const& key) {
    if (!object.HasKey(key)) return std::nullopt;

    auto value = object.GetNamedValue(key);
    if (value.ValueType() != JsonValueType::Boolean) return std::nullopt;
    return value.GetBoolean();
}

bool isNullField(JsonObject const& object, std::wstring const& key) {
    return object.HasKey(key) && object.GetNamedValue(key).ValueType() == JsonValueType::Null;
}

}
