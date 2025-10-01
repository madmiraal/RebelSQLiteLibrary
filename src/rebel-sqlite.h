// SPDX-FileCopyrightText: 2025 Rebel SQLite Library contributors
// SPDX-FileCopyrightText: 2019-2020 TGRCDev
// SPDX-FileCopyrightText: 2017-2019 Khairul Hidayat
//
// SPDX-License-Identifier: MIT

#ifndef REBEL_SQLITE_H
#define REBEL_SQLITE_H

#include "classes/reference.h"
#include "common/rebel.h"
#include "third-party/spmemvfs/spmemvfs.h"
#include "third-party/sqleet/sqleet.h"

namespace Rebel {
enum class Result {
    BOTH = 0,
    NUM,
    ASSOC
};

class SQLite : public Reference {
    REBEL_CLASS(SQLite, Reference)

public:
    ~SQLite();

    void init() {}

    bool open(String path);
    bool open_with_flags(String path, int flags);
    bool open_buffered(String path, PoolByteArray data, int64_t size);
    bool open_buffered_with_flags(
        String path,
        PoolByteArray data,
        int64_t size,
        int flags
    );
    bool open_encrypted(String path, String password);
    bool open_encrypted_with_flags(String path, String password, int flags);
    void close();

    bool query(String statement) const;
    bool query_with_args(String statement, Array args) const;
    bool query_all(String statement_list) const;
    Array fetch_array(String statement) const;
    Array fetch_array_with_args(String statement, Array args) const;
    Array fetch_assoc(String statement) const;
    Array fetch_assoc_with_args(String statement, Array args) const;

    static void register_methods();

private:
    spmemvfs_db_t memory_db = {};
    sqlite3* db             = nullptr;
    bool in_memory          = false;

    sqlite3* get_database() const {
        return (in_memory ? memory_db.handle : db);
    }
};
} // namespace Rebel

#endif // REBEL_SQLITE_H
