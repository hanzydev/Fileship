#pragma once

#include <shared/json.h>

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace fileship::host::harbor {

class SerialQueue {
public:
    explicit SerialQueue(std::wstring name);
    ~SerialQueue();

    std::shared_future<void> post(std::function<void()> task);
    void drain();

private:
    void run();

    std::wstring name;
    std::mutex mutex;
    std::condition_variable wake;
    std::deque<std::pair<std::function<void()>, std::shared_ptr<std::promise<void>>>> tasks;
    std::shared_future<void> last;
    std::thread worker;
};

class Timers {
public:
    Timers();
    ~Timers();

    uint64_t set(int delayMs, std::function<void()> callback);
    void clear(uint64_t id);

private:
    void run();

    using Clock = std::chrono::steady_clock;

    std::mutex mutex;
    std::condition_variable wake;
    std::multimap<Clock::time_point, std::pair<uint64_t, std::function<void()>>> pending;
    uint64_t nextId = 1;
    bool stopping = false;
    std::thread worker;
};

template <typename Value>
class InsertionMap {
public:
    using Item = std::pair<std::wstring, Value>;

    Value* find(std::wstring const& key) {
        auto found = index.find(key);
        return found == index.end() ? nullptr : &found->second->second;
    }

    bool has(std::wstring const& key) const { return index.contains(key); }

    void set(std::wstring const& key, Value value) {
        if (auto existing = find(key)) {
            *existing = std::move(value);
            return;
        }

        items.emplace_back(key, std::move(value));
        index[key] = std::prev(items.end());
    }

    bool erase(std::wstring const& key) {
        auto found = index.find(key);
        if (found == index.end()) return false;

        items.erase(found->second);
        index.erase(found);
        return true;
    }

    void clear() {
        items.clear();
        index.clear();
    }

    size_t size() const { return items.size(); }
    auto begin() { return items.begin(); }
    auto end() { return items.end(); }
    std::list<Item> const& entries() const { return items; }

private:
    std::list<Item> items;
    std::unordered_map<std::wstring, typename std::list<Item>::iterator> index;
};

Timers& timers();

int64_t nowMs();
int64_t parseIsoDate(std::wstring const& text);
std::wstring randomHex(size_t bytes);
std::string sha1Hex(std::string const& data);
std::string sha256Hex(std::string const& data);
std::wstring lower(std::wstring const& text);
bool equalsIgnoreCase(std::wstring const& a, std::wstring const& b);
bool startsWith(std::wstring const& text, std::wstring const& prefix);

std::wstring dirnameOf(std::wstring const& path);
std::wstring basenameOf(std::wstring const& path);
std::wstring extnameOf(std::wstring const& path);

std::wstring formatSize(int64_t bytes);
std::wstring formatNumber(double value, int maxFractionDigits = 0);
std::wstring formatDateTime(int64_t time);
std::wstring relativeTime(int64_t value, int64_t now, std::wstring const& justNow);

std::wstring errorMessage(std::exception const& error);
std::wstring mimeTypeOf(std::wstring const& path);

std::optional<std::wstring> stringField(JsonObject const& object, std::wstring const& key);
std::optional<double> numberField(JsonObject const& object, std::wstring const& key);
std::optional<bool> boolField(JsonObject const& object, std::wstring const& key);
bool isNullField(JsonObject const& object, std::wstring const& key);

}
