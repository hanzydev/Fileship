#include "app.h"
#include "rpc.h"

#include <shared/log.h>

#include <winsqlite/winsqlite3.h>

#include <mutex>

namespace fileship::host {

namespace {

constexpr wchar_t const* COLUMNS[] = {L"id",     L"kind", L"name",      L"localPath", L"url",      L"shortUrl",
                                      L"fileId", L"size", L"thumbnail", L"error",     L"createdAt"};

std::mutex mutex;
sqlite3* database = nullptr;

class Statement {
public:
    Statement(sqlite3* db, std::wstring const& sql) {
        if (sqlite3_prepare16_v2(db, sql.c_str(), -1, &statement, nullptr) != SQLITE_OK)
            throw std::runtime_error(std::string("History query failed: ") + sqlite3_errmsg(db));
    }

    ~Statement() {
        sqlite3_finalize(statement);
    }

    Statement(Statement const&) = delete;
    Statement& operator=(Statement const&) = delete;

    void bind(int index, std::optional<std::wstring> const& value) {
        if (value) sqlite3_bind_text16(statement, index, value->c_str(), -1, SQLITE_TRANSIENT);
        else sqlite3_bind_null(statement, index);
    }

    void bind(int index, int64_t value) {
        sqlite3_bind_int64(statement, index, value);
    }

    bool step() {
        int result = sqlite3_step(statement);
        if (result == SQLITE_ROW) return true;
        if (result != SQLITE_DONE) throw std::runtime_error("History query failed");
        return false;
    }

    JsonObject row() {
        JsonObject item;

        for (int index = 0; index < sqlite3_column_count(statement); index++) {
            std::wstring name(static_cast<wchar_t const*>(sqlite3_column_name16(statement, index)));

            switch (sqlite3_column_type(statement, index)) {
            case SQLITE_NULL:
                item.SetNamedValue(name, JsonValue::CreateNullValue());
                break;
            case SQLITE_INTEGER:
            case SQLITE_FLOAT:
                item.SetNamedValue(name, JsonValue::CreateNumberValue(sqlite3_column_double(statement, index)));
                break;
            default:
                item.SetNamedValue(name, JsonValue::CreateStringValue(
                                             static_cast<wchar_t const*>(sqlite3_column_text16(statement, index))));
            }
        }

        return item;
    }

    int64_t integer(int index) {
        return sqlite3_column_int64(statement, index);
    }

private:
    sqlite3_stmt* statement = nullptr;
};

sqlite3* db() {
    if (database) return database;

    auto path = userDataPath() / L"history.db";
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);

    if (sqlite3_open16(path.c_str(), &database) != SQLITE_OK) {
        sqlite3_close(database);
        database = nullptr;
        throw std::runtime_error("Could not open the history");
    }

    sqlite3_busy_timeout(database, 5000);
    sqlite3_exec(database,
                 "CREATE TABLE IF NOT EXISTS history ("
                 "id TEXT PRIMARY KEY, kind TEXT NOT NULL, name TEXT NOT NULL, localPath TEXT, url TEXT, "
                 "shortUrl TEXT, fileId TEXT, size INTEGER NOT NULL DEFAULT 0, thumbnail TEXT, error TEXT, "
                 "createdAt INTEGER NOT NULL);"
                 "CREATE INDEX IF NOT EXISTS history_createdAt ON history (createdAt DESC);",
                 nullptr, nullptr, nullptr);

    return database;
}

void changed() {
    broadcast(L"history:changed", JsonValue::CreateNullValue());
}

}

std::wstring addHistory(HistoryEntry const& entry) {
    auto id = randomUuid();

    {
        std::lock_guard lock(mutex);
        Statement statement(db(), L"INSERT INTO history (id, kind, name, localPath, url, shortUrl, fileId, size, "
                                  L"thumbnail, error, createdAt) VALUES (?, ?, ?, ?, NULL, NULL, NULL, ?, ?, NULL, ?)");
        statement.bind(1, id);
        statement.bind(2, entry.kind);
        statement.bind(3, entry.name);
        statement.bind(4, entry.localPath);
        statement.bind(5, entry.size);
        statement.bind(6, entry.thumbnail);
        statement.bind(7, nowMs());
        statement.step();
    }

    changed();
    return id;
}

void updateHistory(std::wstring const& id, std::map<std::wstring, std::optional<std::wstring>> const& patch) {
    if (patch.empty()) return;

    std::wstring assignments;
    for (auto const& [key, value] : patch) {
        if (std::find(std::begin(COLUMNS), std::end(COLUMNS), key) == std::end(COLUMNS)) continue;
        assignments += (assignments.empty() ? L"" : L", ") + key + L" = ?";
    }

    {
        std::lock_guard lock(mutex);
        Statement statement(db(), L"UPDATE history SET " + assignments + L" WHERE id = ?");

        int index = 1;
        for (auto const& [key, value] : patch) {
            if (std::find(std::begin(COLUMNS), std::end(COLUMNS), key) == std::end(COLUMNS)) continue;
            statement.bind(index++, value);
        }

        statement.bind(index, id);
        statement.step();
    }

    changed();
}

JsonObject listHistory(int64_t offset, int64_t limit) {
    std::lock_guard lock(mutex);
    JsonArray items;

    {
        Statement statement(db(), L"SELECT * FROM history ORDER BY createdAt DESC LIMIT ? OFFSET ?");
        statement.bind(1, limit);
        statement.bind(2, offset);
        while (statement.step()) items.Append(statement.row());
    }

    Statement count(db(), L"SELECT COUNT(*) AS total FROM history");
    count.step();

    JsonObject result;
    result.SetNamedValue(L"items", items);
    result.SetNamedValue(L"total", JsonValue::CreateNumberValue(static_cast<double>(count.integer(0))));
    return result;
}

std::optional<JsonObject> getHistory(std::wstring const& id) {
    std::lock_guard lock(mutex);
    Statement statement(db(), L"SELECT * FROM history WHERE id = ?");
    statement.bind(1, id);
    if (!statement.step()) return std::nullopt;
    return statement.row();
}

void removeHistory(std::wstring const& id) {
    {
        std::lock_guard lock(mutex);
        Statement statement(db(), L"DELETE FROM history WHERE id = ?");
        statement.bind(1, id);
        statement.step();
    }

    changed();
}

void clearHistory() {
    {
        std::lock_guard lock(mutex);
        sqlite3_exec(db(), "DELETE FROM history", nullptr, nullptr, nullptr);
    }

    changed();
}

}
