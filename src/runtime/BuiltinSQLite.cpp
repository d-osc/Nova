// Nova SQLite Module - Node.js compatible sqlite API (Node.js 22.5.0+)
// Provides synchronous SQLite database operations

#include <string>
#include <vector>
#include <map>
#include <cstring>
#include <cstdint>
#include <algorithm>

// Note: This implementation provides the API structure.
// For full functionality, link with SQLite3 library.
// Stub implementations are provided for compilation without SQLite3.

#ifdef NOVA_HAS_SQLITE3
#include <sqlite3.h>
#else
// Stub types when SQLite3 is not available
typedef void sqlite3;
typedef void sqlite3_stmt;
#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101
#define SQLITE_OPEN_READWRITE 0x00000002
#define SQLITE_OPEN_CREATE 0x00000004
#define SQLITE_OPEN_MEMORY 0x00000080
#define SQLITE_OPEN_READONLY 0x00000001
#endif

// Database open modes
static const int SQLITE_MODE_READWRITE = 1;
static const int SQLITE_MODE_READONLY = 2;
static const int SQLITE_MODE_CREATE = 4;
static const int SQLITE_MODE_MEMORY = 8;

// Column types
static const int SQLITE_TYPE_NULL = 0;
static const int SQLITE_TYPE_INTEGER = 1;
static const int SQLITE_TYPE_FLOAT = 2;
static const int SQLITE_TYPE_TEXT = 3;
static const int SQLITE_TYPE_BLOB = 4;

// Statement result row
struct SQLiteRow {
    std::map<std::string, std::string> columns;
    std::vector<std::string> columnNames;
    std::vector<int> columnTypes;
};

// Statement object
struct SQLiteStatement {
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::string sql;
    std::string expandedSQL;
    bool allowBareNamedParams;
    bool readBigInts;
    std::vector<SQLiteRow> results;
    int lastChanges;
    int64_t lastInsertRowId;
};

// Database object
struct SQLiteDatabase {
    sqlite3* db;
    std::string location;
    bool isOpen;
    bool isMemory;
    bool isReadOnly;
    std::string lastError;
    std::vector<SQLiteStatement*> statements;
};

// Session object for tracking changes
struct SQLiteSession {
    void* session;           // sqlite3_session*
    SQLiteDatabase* db;
    std::string tableName;
    bool isAttached;
    std::vector<uint8_t> changeset;
};

// Active databases
static std::vector<SQLiteDatabase*> databases;
static std::vector<SQLiteSession*> sessions;

extern "C" {

// ============================================================================
// DatabaseSync Class
// ============================================================================

// new DatabaseSync(location, options)
void* nova_sqlite_Database_new(const char* location, int mode, bool enableForeignKeys) {
    auto* db = new SQLiteDatabase();
    db->location = location ? location : ":memory:";
    db->isOpen = false;
    db->isMemory = (db->location == ":memory:");
    db->isReadOnly = (mode & SQLITE_MODE_READONLY) != 0;
    db->db = nullptr;

#ifdef NOVA_HAS_SQLITE3
    int flags = 0;
    if (mode & SQLITE_MODE_READONLY) {
        flags |= SQLITE_OPEN_READONLY;
    } else {
        flags |= SQLITE_OPEN_READWRITE;
        if (mode & SQLITE_MODE_CREATE) {
            flags |= SQLITE_OPEN_CREATE;
        }
    }
    if (mode & SQLITE_MODE_MEMORY) {
        flags |= SQLITE_OPEN_MEMORY;
    }

    int rc = sqlite3_open_v2(db->location.c_str(), &db->db, flags, nullptr);
    if (rc == SQLITE_OK) {
        db->isOpen = true;

        if (enableForeignKeys) {
            sqlite3_exec(db->db, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
        }
    } else {
        db->lastError = sqlite3_errmsg(db->db);
    }
#else
    db->isOpen = true;  // Stub: pretend it's open
    (void)enableForeignKeys;
#endif

    databases.push_back(db);
    return db;
}

// Open with default options
void* nova_sqlite_Database_open(const char* location) {
    return nova_sqlite_Database_new(location, SQLITE_MODE_READWRITE | SQLITE_MODE_CREATE, true);
}

// Open in-memory database
void* nova_sqlite_Database_openMemory() {
    return nova_sqlite_Database_new(":memory:", SQLITE_MODE_MEMORY | SQLITE_MODE_READWRITE | SQLITE_MODE_CREATE, true);
}

// database.close()
void nova_sqlite_Database_close(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    if (!db->isOpen) return;

#ifdef NOVA_HAS_SQLITE3
    // Finalize all statements
    for (auto* stmt : db->statements) {
        if (stmt->stmt) {
            sqlite3_finalize(stmt->stmt);
            stmt->stmt = nullptr;
        }
    }

    sqlite3_close(db->db);
#endif

    db->db = nullptr;
    db->isOpen = false;
}

// database.open()
bool nova_sqlite_Database_reopen(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    if (db->isOpen) return true;

#ifdef NOVA_HAS_SQLITE3
    int flags = db->isReadOnly ? SQLITE_OPEN_READONLY :
                (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);

    int rc = sqlite3_open_v2(db->location.c_str(), &db->db, flags, nullptr);
    if (rc == SQLITE_OK) {
        db->isOpen = true;
        return true;
    } else {
        db->lastError = sqlite3_errmsg(db->db);
        return false;
    }
#else
    db->isOpen = true;
    return true;
#endif
}

// database.exec(sql)
bool nova_sqlite_Database_exec(void* database, const char* sql) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    if (!db->isOpen || !sql) return false;

#ifdef NOVA_HAS_SQLITE3
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db->db, sql, nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        if (errMsg) {
            db->lastError = errMsg;
            sqlite3_free(errMsg);
        }
        return false;
    }
    return true;
#else
    (void)sql;
    return true;
#endif
}

// database.prepare(sql) → StatementSync
void* nova_sqlite_Database_prepare(void* database, const char* sql) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    if (!db->isOpen || !sql) return nullptr;

    auto* stmt = new SQLiteStatement();
    stmt->db = db->db;
    stmt->stmt = nullptr;
    stmt->sql = sql;
    stmt->allowBareNamedParams = false;
    stmt->readBigInts = false;
    stmt->lastChanges = 0;
    stmt->lastInsertRowId = 0;

#ifdef NOVA_HAS_SQLITE3
    int rc = sqlite3_prepare_v2(db->db, sql, -1, &stmt->stmt, nullptr);
    if (rc != SQLITE_OK) {
        db->lastError = sqlite3_errmsg(db->db);
        delete stmt;
        return nullptr;
    }

    // Get expanded SQL
    const char* expanded = sqlite3_expanded_sql(stmt->stmt);
    if (expanded) {
        stmt->expandedSQL = expanded;
        sqlite3_free((void*)expanded);
    }
#else
    stmt->expandedSQL = sql;
#endif

    db->statements.push_back(stmt);
    return stmt;
}

// database.enableLoadExtension(allow)
void nova_sqlite_Database_enableLoadExtension(void* database, bool allow) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    if (!db->isOpen) return;

#ifdef NOVA_HAS_SQLITE3
    sqlite3_enable_load_extension(db->db, allow ? 1 : 0);
#else
    (void)allow;
#endif
}

// database.loadExtension(path)
bool nova_sqlite_Database_loadExtension(void* database, const char* path) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    if (!db->isOpen || !path) return false;

#ifdef NOVA_HAS_SQLITE3
    char* errMsg = nullptr;
    int rc = sqlite3_load_extension(db->db, path, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        if (errMsg) {
            db->lastError = errMsg;
            sqlite3_free(errMsg);
        }
        return false;
    }
    return true;
#else
    (void)path;
    return false;
#endif
}

// Get last error
const char* nova_sqlite_Database_lastError(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    return db->lastError.c_str();
}

// Check if open
bool nova_sqlite_Database_isOpen(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    return db->isOpen;
}

// Get location
const char* nova_sqlite_Database_location(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    return db->location.c_str();
}

// Check if in-memory
bool nova_sqlite_Database_isMemory(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    return db->isMemory;
}

// Check if read-only
bool nova_sqlite_Database_isReadOnly(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);
    return db->isReadOnly;
}

// Free database
void nova_sqlite_Database_free(void* database) {
    auto* db = static_cast<SQLiteDatabase*>(database);

    nova_sqlite_Database_close(database);

    // Free statements
    for (auto* stmt : db->statements) {
        delete stmt;
    }

    auto it = std::find(databases.begin(), databases.end(), db);
    if (it != databases.end()) {
        databases.erase(it);
    }

    delete db;
}

// ============================================================================
// StatementSync Class
// ============================================================================

// statement.run(params...) - Execute and return changes info
void* nova_sqlite_Statement_run(void* statement, const char** paramNames,
                                 const char** paramValues, int paramCount) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    if (!stmt->stmt) return nullptr;

#ifdef NOVA_HAS_SQLITE3
    sqlite3_reset(stmt->stmt);
    sqlite3_clear_bindings(stmt->stmt);

    // Bind parameters
    for (int i = 0; i < paramCount; i++) {
        if (paramNames && paramNames[i]) {
            // Named parameter
            std::string name = paramNames[i];
            if (name[0] != ':' && name[0] != '@' && name[0] != '$') {
                if (stmt->allowBareNamedParams) {
                    name = ":" + name;
                }
            }
            int idx = sqlite3_bind_parameter_index(stmt->stmt, name.c_str());
            if (idx > 0 && paramValues[i]) {
                sqlite3_bind_text(stmt->stmt, idx, paramValues[i], -1, SQLITE_TRANSIENT);
            }
        } else if (paramValues[i]) {
            // Positional parameter (1-indexed)
            sqlite3_bind_text(stmt->stmt, i + 1, paramValues[i], -1, SQLITE_TRANSIENT);
        }
    }

    int rc = sqlite3_step(stmt->stmt);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        return nullptr;
    }

    stmt->lastChanges = sqlite3_changes(sqlite3_db_handle(stmt->stmt));
    stmt->lastInsertRowId = sqlite3_last_insert_rowid(sqlite3_db_handle(stmt->stmt));
#else
    (void)paramNames;
    (void)paramValues;
    (void)paramCount;
    stmt->lastChanges = 0;
    stmt->lastInsertRowId = 0;
#endif

    return stmt;
}

// statement.get(params...) - Get single row
void* nova_sqlite_Statement_get(void* statement, const char** paramNames,
                                 const char** paramValues, int paramCount) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    if (!stmt->stmt) return nullptr;

    stmt->results.clear();

#ifdef NOVA_HAS_SQLITE3
    sqlite3_reset(stmt->stmt);
    sqlite3_clear_bindings(stmt->stmt);

    // Bind parameters
    for (int i = 0; i < paramCount; i++) {
        if (paramNames && paramNames[i]) {
            std::string name = paramNames[i];
            if (name[0] != ':' && name[0] != '@' && name[0] != '$') {
                if (stmt->allowBareNamedParams) {
                    name = ":" + name;
                }
            }
            int idx = sqlite3_bind_parameter_index(stmt->stmt, name.c_str());
            if (idx > 0 && paramValues[i]) {
                sqlite3_bind_text(stmt->stmt, idx, paramValues[i], -1, SQLITE_TRANSIENT);
            }
        } else if (paramValues[i]) {
            sqlite3_bind_text(stmt->stmt, i + 1, paramValues[i], -1, SQLITE_TRANSIENT);
        }
    }

    int rc = sqlite3_step(stmt->stmt);
    if (rc == SQLITE_ROW) {
        SQLiteRow row;
        int colCount = sqlite3_column_count(stmt->stmt);

        for (int i = 0; i < colCount; i++) {
            const char* colName = sqlite3_column_name(stmt->stmt, i);
            const char* colValue = (const char*)sqlite3_column_text(stmt->stmt, i);
            int colType = sqlite3_column_type(stmt->stmt, i);

            row.columnNames.push_back(colName ? colName : "");
            row.columns[colName ? colName : ""] = colValue ? colValue : "";
            row.columnTypes.push_back(colType);
        }

        stmt->results.push_back(row);
        return &stmt->results[0];
    }
#else
    (void)paramNames;
    (void)paramValues;
    (void)paramCount;
#endif

    return nullptr;
}

// statement.all(params...) - Get all rows
int nova_sqlite_Statement_all(void* statement, const char** paramNames,
                               const char** paramValues, int paramCount) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    if (!stmt->stmt) return 0;

    stmt->results.clear();

#ifdef NOVA_HAS_SQLITE3
    sqlite3_reset(stmt->stmt);
    sqlite3_clear_bindings(stmt->stmt);

    // Bind parameters
    for (int i = 0; i < paramCount; i++) {
        if (paramNames && paramNames[i]) {
            std::string name = paramNames[i];
            if (name[0] != ':' && name[0] != '@' && name[0] != '$') {
                if (stmt->allowBareNamedParams) {
                    name = ":" + name;
                }
            }
            int idx = sqlite3_bind_parameter_index(stmt->stmt, name.c_str());
            if (idx > 0 && paramValues[i]) {
                sqlite3_bind_text(stmt->stmt, idx, paramValues[i], -1, SQLITE_TRANSIENT);
            }
        } else if (paramValues[i]) {
            sqlite3_bind_text(stmt->stmt, i + 1, paramValues[i], -1, SQLITE_TRANSIENT);
        }
    }

    int rc;
    while ((rc = sqlite3_step(stmt->stmt)) == SQLITE_ROW) {
        SQLiteRow row;
        int colCount = sqlite3_column_count(stmt->stmt);

        for (int i = 0; i < colCount; i++) {
            const char* colName = sqlite3_column_name(stmt->stmt, i);
            const char* colValue = (const char*)sqlite3_column_text(stmt->stmt, i);
            int colType = sqlite3_column_type(stmt->stmt, i);

            row.columnNames.push_back(colName ? colName : "");
            row.columns[colName ? colName : ""] = colValue ? colValue : "";
            row.columnTypes.push_back(colType);
        }

        stmt->results.push_back(row);
    }
#else
    (void)paramNames;
    (void)paramValues;
    (void)paramCount;
#endif

    return static_cast<int>(stmt->results.size());
}

// Get row by index
void* nova_sqlite_Statement_getRow(void* statement, int index) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    if (index < 0 || index >= static_cast<int>(stmt->results.size())) {
        return nullptr;
    }
    return &stmt->results[index];
}

// Get column value from row
const char* nova_sqlite_Row_getValue(void* row, const char* column) {
    static std::string result;
    auto* r = static_cast<SQLiteRow*>(row);

    auto it = r->columns.find(column);
    if (it != r->columns.end()) {
        result = it->second;
        return result.c_str();
    }
    return nullptr;
}

// Get column value by index
const char* nova_sqlite_Row_getValueByIndex(void* row, int index) {
    static std::string result;
    auto* r = static_cast<SQLiteRow*>(row);

    if (index < 0 || index >= static_cast<int>(r->columnNames.size())) {
        return nullptr;
    }

    result = r->columns[r->columnNames[index]];
    return result.c_str();
}

// Get column names
const char** nova_sqlite_Row_getColumns(void* row, int* count) {
    static std::vector<const char*> cols;
    auto* r = static_cast<SQLiteRow*>(row);

    cols.clear();
    for (const auto& col : r->columnNames) {
        cols.push_back(col.c_str());
    }

    *count = static_cast<int>(cols.size());
    return cols.data();
}

// Get column type
int nova_sqlite_Row_getColumnType(void* row, int index) {
    auto* r = static_cast<SQLiteRow*>(row);
    if (index < 0 || index >= static_cast<int>(r->columnTypes.size())) {
        return SQLITE_TYPE_NULL;
    }
    return r->columnTypes[index];
}

// statement.sourceSQL
const char* nova_sqlite_Statement_sourceSQL(void* statement) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    return stmt->sql.c_str();
}

// statement.expandedSQL
const char* nova_sqlite_Statement_expandedSQL(void* statement) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    return stmt->expandedSQL.c_str();
}

// statement.setAllowBareNamedParameters(enabled)
void nova_sqlite_Statement_setAllowBareNamedParameters(void* statement, bool enabled) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    stmt->allowBareNamedParams = enabled;
}

// statement.setReadBigInts(enabled)
void nova_sqlite_Statement_setReadBigInts(void* statement, bool enabled) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    stmt->readBigInts = enabled;
}

// Get last changes count
int nova_sqlite_Statement_changes(void* statement) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    return stmt->lastChanges;
}

// Get last insert row ID
int64_t nova_sqlite_Statement_lastInsertRowId(void* statement) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    return stmt->lastInsertRowId;
}

// Finalize statement
void nova_sqlite_Statement_finalize(void* statement) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);

#ifdef NOVA_HAS_SQLITE3
    if (stmt->stmt) {
        sqlite3_finalize(stmt->stmt);
        stmt->stmt = nullptr;
    }
#else
    (void)stmt;
#endif
}

// Free statement
void nova_sqlite_Statement_free(void* statement) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    nova_sqlite_Statement_finalize(statement);
    delete stmt;
}

// ============================================================================
// Iterator Support (for statement.iterate())
// ============================================================================

struct SQLiteIterator {
    SQLiteStatement* stmt;
    bool started;
    bool done;
    SQLiteRow currentRow;
};

// Create iterator
void* nova_sqlite_Statement_iterate(void* statement, const char** paramNames,
                                     const char** paramValues, int paramCount) {
    auto* stmt = static_cast<SQLiteStatement*>(statement);
    if (!stmt->stmt) return nullptr;

    auto* iter = new SQLiteIterator();
    iter->stmt = stmt;
    iter->started = false;
    iter->done = false;

#ifdef NOVA_HAS_SQLITE3
    sqlite3_reset(stmt->stmt);
    sqlite3_clear_bindings(stmt->stmt);

    // Bind parameters
    for (int i = 0; i < paramCount; i++) {
        if (paramNames && paramNames[i]) {
            std::string name = paramNames[i];
            if (name[0] != ':' && name[0] != '@' && name[0] != '$') {
                if (stmt->allowBareNamedParams) {
                    name = ":" + name;
                }
            }
            int idx = sqlite3_bind_parameter_index(stmt->stmt, name.c_str());
            if (idx > 0 && paramValues[i]) {
                sqlite3_bind_text(stmt->stmt, idx, paramValues[i], -1, SQLITE_TRANSIENT);
            }
        } else if (paramValues[i]) {
            sqlite3_bind_text(stmt->stmt, i + 1, paramValues[i], -1, SQLITE_TRANSIENT);
        }
    }
#else
    (void)paramNames;
    (void)paramValues;
    (void)paramCount;
    iter->done = true;
#endif

    return iter;
}

// Get next row from iterator
void* nova_sqlite_Iterator_next(void* iterator) {
    auto* iter = static_cast<SQLiteIterator*>(iterator);
    if (iter->done) return nullptr;

#ifdef NOVA_HAS_SQLITE3
    int rc = sqlite3_step(iter->stmt->stmt);
    if (rc == SQLITE_ROW) {
        iter->currentRow.columns.clear();
        iter->currentRow.columnNames.clear();
        iter->currentRow.columnTypes.clear();

        int colCount = sqlite3_column_count(iter->stmt->stmt);
        for (int i = 0; i < colCount; i++) {
            const char* colName = sqlite3_column_name(iter->stmt->stmt, i);
            const char* colValue = (const char*)sqlite3_column_text(iter->stmt->stmt, i);
            int colType = sqlite3_column_type(iter->stmt->stmt, i);

            iter->currentRow.columnNames.push_back(colName ? colName : "");
            iter->currentRow.columns[colName ? colName : ""] = colValue ? colValue : "";
            iter->currentRow.columnTypes.push_back(colType);
        }

        return &iter->currentRow;
    }
#endif

    iter->done = true;
    return nullptr;
}

// Check if iterator is done
bool nova_sqlite_Iterator_done(void* iterator) {
    auto* iter = static_cast<SQLiteIterator*>(iterator);
    return iter->done;
}

// Free iterator
void nova_sqlite_Iterator_free(void* iterator) {
    delete static_cast<SQLiteIterator*>(iterator);
}

// ============================================================================
// Session/Changeset Support (Node.js 22.5.0+ experimental)
// ============================================================================

// database.createSession(options)
void* nova_sqlite_Database_createSession(void* database, const char* tableName) {
    if (!database) return nullptr;
    SQLiteDatabase* db = static_cast<SQLiteDatabase*>(database);

    auto* session = new SQLiteSession();
    session->db = db;
    session->tableName = tableName ? tableName : "";
    session->isAttached = false;
    session->session = nullptr;

#ifdef NOVA_HAS_SQLITE3
    // Create session using SQLite session extension
    // Note: Requires SQLite compiled with -DSQLITE_ENABLE_SESSION
    #ifdef SQLITE_ENABLE_SESSION
    int rc = sqlite3session_create(db->db, "main", (sqlite3_session**)&session->session);
    if (rc == SQLITE_OK && session->session) {
        if (tableName && tableName[0] != '\0') {
            sqlite3session_attach((sqlite3_session*)session->session, tableName);
        } else {
            sqlite3session_attach((sqlite3_session*)session->session, nullptr);  // All tables
        }
        session->isAttached = true;
    }
    #endif
#endif

    sessions.push_back(session);
    return session;
}

// session.changeset() - Get the changeset from the session
void* nova_sqlite_Session_changeset(void* sessionPtr, int* size) {
    if (!sessionPtr || !size) {
        if (size) *size = 0;
        return nullptr;
    }

    SQLiteSession* session = static_cast<SQLiteSession*>(sessionPtr);
    (void)session;  // Used conditionally
    *size = 0;

#ifdef NOVA_HAS_SQLITE3
    #ifdef SQLITE_ENABLE_SESSION
    if (session->session) {
        void* changeset = nullptr;
        int nChangeset = 0;
        int rc = sqlite3session_changeset((sqlite3_session*)session->session, &nChangeset, &changeset);
        if (rc == SQLITE_OK && changeset && nChangeset > 0) {
            // Copy changeset data
            uint8_t* result = new uint8_t[nChangeset];
            memcpy(result, changeset, nChangeset);
            sqlite3_free(changeset);
            *size = nChangeset;
            return result;
        }
    }
    #endif
#endif

    // Return empty changeset if session extension not available
    return nullptr;
}

// session.patchset() - Get the patchset (more compact than changeset)
void* nova_sqlite_Session_patchset(void* sessionPtr, int* size) {
    if (!sessionPtr || !size) {
        if (size) *size = 0;
        return nullptr;
    }

    SQLiteSession* session = static_cast<SQLiteSession*>(sessionPtr);
    (void)session;  // Used conditionally
    *size = 0;

#ifdef NOVA_HAS_SQLITE3
    #ifdef SQLITE_ENABLE_SESSION
    if (session->session) {
        void* patchset = nullptr;
        int nPatchset = 0;
        int rc = sqlite3session_patchset((sqlite3_session*)session->session, &nPatchset, &patchset);
        if (rc == SQLITE_OK && patchset && nPatchset > 0) {
            uint8_t* result = new uint8_t[nPatchset];
            memcpy(result, patchset, nPatchset);
            sqlite3_free(patchset);
            *size = nPatchset;
            return result;
        }
    }
    #endif
#endif

    return nullptr;
}

// session.close() - Close the session
void nova_sqlite_Session_close(void* sessionPtr) {
    if (!sessionPtr) return;

    SQLiteSession* session = static_cast<SQLiteSession*>(sessionPtr);

#ifdef NOVA_HAS_SQLITE3
    #ifdef SQLITE_ENABLE_SESSION
    if (session->session) {
        sqlite3session_delete((sqlite3_session*)session->session);
        session->session = nullptr;
    }
    #endif
#endif

    // Remove from sessions list
    auto it = std::find(sessions.begin(), sessions.end(), session);
    if (it != sessions.end()) {
        sessions.erase(it);
    }
    delete session;
}

// database.applyChangeset(changeset, options)
bool nova_sqlite_Database_applyChangeset(void* database, const void* changeset, int size) {
    if (!database || !changeset || size <= 0) return false;

    SQLiteDatabase* db = static_cast<SQLiteDatabase*>(database);

#ifdef NOVA_HAS_SQLITE3
    #ifdef SQLITE_ENABLE_SESSION
    int rc = sqlite3changeset_apply(db->db, size, (void*)changeset, nullptr, nullptr, nullptr);
    return rc == SQLITE_OK;
    #else
    (void)db;
    return false;  // Session extension not compiled in
    #endif
#else
    (void)db;
    return false;  // SQLite not available
#endif
}

// database.applyChangeset with conflict handler
bool nova_sqlite_Database_applyChangesetWithHandler(void* database, const void* changeset, int size,
                                                      int (*conflictHandler)(int)) {
    if (!database || !changeset || size <= 0) return false;

    SQLiteDatabase* db = static_cast<SQLiteDatabase*>(database);

#ifdef NOVA_HAS_SQLITE3
    #ifdef SQLITE_ENABLE_SESSION
    // Conflict callback wrapper
    auto callback = [](void* ctx, int conflict, sqlite3_changeset_iter* iter) -> int {
        (void)iter;
        int (*handler)(int) = (int(*)(int))ctx;
        if (handler) {
            return handler(conflict);
        }
        return SQLITE_CHANGESET_ABORT;
    };

    int rc = sqlite3changeset_apply(db->db, size, (void*)changeset, nullptr, callback, (void*)conflictHandler);
    return rc == SQLITE_OK;
    #else
    (void)db;
    (void)conflictHandler;
    return false;
    #endif
#else
    (void)db;
    (void)conflictHandler;
    return false;
#endif
}

// Invert a changeset
void* nova_sqlite_invertChangeset(const void* changeset, int size, int* outSize) {
    if (!changeset || size <= 0 || !outSize) {
        if (outSize) *outSize = 0;
        return nullptr;
    }

#ifdef NOVA_HAS_SQLITE3
    #ifdef SQLITE_ENABLE_SESSION
    void* inverted = nullptr;
    int nInverted = 0;
    int rc = sqlite3changeset_invert(size, (void*)changeset, &nInverted, &inverted);
    if (rc == SQLITE_OK && inverted && nInverted > 0) {
        uint8_t* result = new uint8_t[nInverted];
        memcpy(result, inverted, nInverted);
        sqlite3_free(inverted);
        *outSize = nInverted;
        return result;
    }
    #endif
#endif

    *outSize = 0;
    return nullptr;
}

// Concatenate two changesets
void* nova_sqlite_concatChangesets(const void* cs1, int size1, const void* cs2, int size2, int* outSize) {
    if (!cs1 || !cs2 || size1 <= 0 || size2 <= 0 || !outSize) {
        if (outSize) *outSize = 0;
        return nullptr;
    }

#ifdef NOVA_HAS_SQLITE3
    #ifdef SQLITE_ENABLE_SESSION
    sqlite3_changegroup* group = nullptr;
    int rc = sqlite3changegroup_new(&group);
    if (rc == SQLITE_OK && group) {
        rc = sqlite3changegroup_add(group, size1, (void*)cs1);
        if (rc == SQLITE_OK) {
            rc = sqlite3changegroup_add(group, size2, (void*)cs2);
        }
        if (rc == SQLITE_OK) {
            void* output = nullptr;
            int nOutput = 0;
            rc = sqlite3changegroup_output(group, &nOutput, &output);
            if (rc == SQLITE_OK && output && nOutput > 0) {
                uint8_t* result = new uint8_t[nOutput];
                memcpy(result, output, nOutput);
                sqlite3_free(output);
                sqlite3changegroup_delete(group);
                *outSize = nOutput;
                return result;
            }
        }
        sqlite3changegroup_delete(group);
    }
    #endif
#endif

    *outSize = 0;
    return nullptr;
}

// Free changeset memory
void nova_sqlite_freeChangeset(void* changeset) {
    if (changeset) {
        delete[] static_cast<uint8_t*>(changeset);
    }
}

// ============================================================================
// Constants
// ============================================================================

int nova_sqlite_MODE_READWRITE() { return SQLITE_MODE_READWRITE; }
int nova_sqlite_MODE_READONLY() { return SQLITE_MODE_READONLY; }
int nova_sqlite_MODE_CREATE() { return SQLITE_MODE_CREATE; }
int nova_sqlite_MODE_MEMORY() { return SQLITE_MODE_MEMORY; }

int nova_sqlite_TYPE_NULL() { return SQLITE_TYPE_NULL; }
int nova_sqlite_TYPE_INTEGER() { return SQLITE_TYPE_INTEGER; }
int nova_sqlite_TYPE_FLOAT() { return SQLITE_TYPE_FLOAT; }
int nova_sqlite_TYPE_TEXT() { return SQLITE_TYPE_TEXT; }
int nova_sqlite_TYPE_BLOB() { return SQLITE_TYPE_BLOB; }

// ============================================================================
// Cleanup
// ============================================================================

void nova_sqlite_cleanup() {
    for (auto* db : databases) {
        nova_sqlite_Database_close(db);
        for (auto* stmt : db->statements) {
            delete stmt;
        }
        delete db;
    }
    databases.clear();
}

} // extern "C"
