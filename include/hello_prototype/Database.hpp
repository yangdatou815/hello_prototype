#ifndef DATABASE_HPP
#define DATABASE_HPP

#include "Peer.hpp"
#include <sqlite3.h>
#include <string>
#include <vector>

// ─────────────────────────────────────────────────────────────
// Database: SQLite3 local store for each peer endpoint.
//
// Tables:
//   peer_status  – current state of the remote peer
//   messages     – decrypted messages received from the peer
// ─────────────────────────────────────────────────────────────
struct MessageRecord {
    int         id;
    std::string sender;     // "client" or "server"
    std::string content;    // decrypted plaintext
    std::string timestamp;  // ISO-8601
};

class Database {
public:
    explicit Database(const std::string& db_path);
    ~Database();

    // Peer state
    void        updatePeerState(const std::string& peer_id, PeerState state);
    PeerState   getPeerState(const std::string& peer_id) const;

    // Messages
    void insertMessage(const std::string& sender, const std::string& content);
    std::vector<MessageRecord> getMessages() const;

private:
    void exec(const std::string& sql) const;
    void createTables();

    sqlite3*    db_ = nullptr;
    std::string db_path_;
};

#endif // DATABASE_HPP
