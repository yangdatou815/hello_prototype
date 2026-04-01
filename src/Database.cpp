#include <hello_prototype/Database.hpp>
#include <iostream>
#include <stdexcept>
#include <ctime>
#include <iomanip>
#include <sstream>

// ── helpers ──────────────────────────────────────────────────
static std::string stateToStr(PeerState s) {
    switch (s) {
        case PeerState::OFFLINE:   return "OFFLINE";
        case PeerState::DETECTED:  return "DETECTED";
        case PeerState::CONNECTED: return "CONNECTED";
        case PeerState::ONLINE:    return "ONLINE";
        default:                   return "UNKNOWN";
    }
}

static PeerState strToState(const std::string& s) {
    if (s == "DETECTED")  return PeerState::DETECTED;
    if (s == "CONNECTED") return PeerState::CONNECTED;
    if (s == "ONLINE")    return PeerState::ONLINE;
    return PeerState::OFFLINE;
}

static std::string nowIso() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
    localtime_r(&t, &tm);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

// ── ctor / dtor ───────────────────────────────────────────────
Database::Database(const std::string& db_path) : db_path_(db_path) {
    if (sqlite3_open(db_path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("[DB] Failed to open: " + db_path
                                 + " (" + sqlite3_errmsg(db_) + ")");
    }
    std::cout << "[DB] Opened: " << db_path << std::endl;
    createTables();
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        std::cout << "[DB] Closed: " << db_path_ << std::endl;
    }
}

// ── private helpers ───────────────────────────────────────────
void Database::exec(const std::string& sql) const {
    char* errmsg = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK) {
        std::string err(errmsg ? errmsg : "unknown");
        sqlite3_free(errmsg);
        throw std::runtime_error("[DB] SQL error: " + err + "\nSQL: " + sql);
    }
}

void Database::createTables() {
    exec(R"(
        CREATE TABLE IF NOT EXISTS peer_status (
            peer_id   TEXT    PRIMARY KEY,
            state     TEXT    NOT NULL DEFAULT 'OFFLINE',
            updated   TEXT    NOT NULL
        );
    )");
    exec(R"(
        CREATE TABLE IF NOT EXISTS messages (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            sender    TEXT    NOT NULL,
            content   TEXT    NOT NULL,
            timestamp TEXT    NOT NULL
        );
    )");
    std::cout << "[DB] Tables ready (peer_status, messages)." << std::endl;
}

// ── peer state ────────────────────────────────────────────────
void Database::updatePeerState(const std::string& peer_id, PeerState state) {
    std::string s   = stateToStr(state);
    std::string now = nowIso();
    // UPSERT: insert or replace
    std::string sql =
        "INSERT INTO peer_status (peer_id, state, updated) VALUES ('"
        + peer_id + "','" + s + "','" + now + "')"
        " ON CONFLICT(peer_id) DO UPDATE SET state='" + s + "', updated='" + now + "';";
    exec(sql);
    std::cout << "[DB] Peer '" << peer_id << "' state -> " << s
              << " at " << now << std::endl;
}

PeerState Database::getPeerState(const std::string& peer_id) const {
    std::string result = "OFFLINE";
    std::string sql = "SELECT state FROM peer_status WHERE peer_id='" + peer_id + "';";

    auto cb = [](void* data, int, char** vals, char**) -> int {
        if (vals[0]) *static_cast<std::string*>(data) = vals[0];
        return 0;
    };
    char* errmsg = nullptr;
    sqlite3_exec(db_, sql.c_str(), cb, &result, &errmsg);
    sqlite3_free(errmsg);
    return strToState(result);
}

// ── messages ──────────────────────────────────────────────────
void Database::insertMessage(const std::string& sender, const std::string& content) {
    std::string now = nowIso();
    // Escape single quotes in content
    std::string safe;
    for (char c : content) {
        if (c == '\'') safe += "''";
        else           safe += c;
    }
    std::string sql =
        "INSERT INTO messages (sender, content, timestamp) VALUES ('"
        + sender + "','" + safe + "','" + now + "');";
    exec(sql);
    std::cout << "[DB] Message saved. sender=" << sender
              << " content=\"" << content << "\" at " << now << std::endl;
}

std::vector<MessageRecord> Database::getMessages() const {
    std::vector<MessageRecord> records;

    auto cb = [](void* data, int, char** vals, char**) -> int {
        auto* v = static_cast<std::vector<MessageRecord>*>(data);
        MessageRecord r;
        r.id        = vals[0] ? std::stoi(vals[0]) : 0;
        r.sender    = vals[1] ? vals[1] : "";
        r.content   = vals[2] ? vals[2] : "";
        r.timestamp = vals[3] ? vals[3] : "";
        v->push_back(r);
        return 0;
    };
    char* errmsg = nullptr;
    sqlite3_exec(db_,
        "SELECT id, sender, content, timestamp FROM messages ORDER BY id;",
        cb, &records, &errmsg);
    sqlite3_free(errmsg);
    return records;
}
