// SPDX-FileCopyrightText: 2025 Rebel SQLite Library contributors
// SPDX-FileCopyrightText: 2019-2020 TGRCDev
// SPDX-FileCopyrightText: 2017-2019 Khairul Hidayat
//
// SPDX-License-Identifier: MIT

#include "rebel-sqlite.h"

#include "classes/engine.h"
#include "classes/file.h"
#include "classes/projectsettings.h"

constexpr int DEFAULT_OPEN_FLAGS = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
constexpr int ALLOWED_FLAGS_MASK =
    SQLITE_OPEN_READONLY | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    | SQLITE_OPEN_URI | SQLITE_OPEN_MEMORY | SQLITE_OPEN_NOMUTEX
    | SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_SHAREDCACHE | SQLITE_OPEN_PRIVATECACHE
    | SQLITE_OPEN_NOFOLLOW;

namespace Rebel {
namespace {
sqlite3_stmt* prepare(sqlite3* database, const char* query) {
    if (!database) {
        Global::print("ERROR: SQL query requires an open database.");
        return nullptr;
    }

    sqlite3_stmt* statement;
    int result = sqlite3_prepare_v2(database, query, -1, &statement, nullptr);
    if (result != SQLITE_OK) {
        Global::print("SQL Error: " + String(sqlite3_errmsg(database)));
        return nullptr;
    }
    return statement;
}

bool bind_args(sqlite3_stmt* statement, Array args) {
    const int arg_count = sqlite3_bind_parameter_count(statement);
    if (arg_count != args.size()) {
        Global::print(String("ERROR: Query argument mismatch! "
                             "Expected % arguments, received %")
                          .format(Array::make(arg_count, args.size()), "%"));
        return false;
    }

    // SQLite data types:
    // - NULL
    // - INTEGER (signed, max 8 bytes)
    // - REAL (stored as a double-precision float)
    // - TEXT (stored in database encoding of UTF-8, UTF-16BE or UTF-16LE)
    // - BLOB (1:1 storage)
    for (int i = 0; i < arg_count; i++) {
        const int index                   = i + 1;
        const Variant& argument           = args[i];
        const Variant::Type argument_type = argument.get_type();
        int error;
        switch (argument_type) {
            case Variant::Type::NIL: {
                error = sqlite3_bind_null(statement, index);
            } break;
            case Variant::Type::BOOL:
            case Variant::Type::INT: {
                error = sqlite3_bind_int(statement, index, argument);
            } break;
            case Variant::Type::REAL: {
                error = sqlite3_bind_double(statement, index, argument);
            } break;
            case Variant::Type::STRING: {
                String string = argument.operator String();
                error         = sqlite3_bind_text(
                    statement,
                    index,
                    string.utf8().get_data(),
                    -1,
                    SQLITE_TRANSIENT
                );
            } break;
            case Variant::Type::POOL_BYTE_ARRAY: {
                PoolByteArray data = argument.operator PoolByteArray();
                error              = sqlite3_bind_blob(
                    statement,
                    index,
                    data.read().ptr(),
                    data.size(),
                    SQLITE_TRANSIENT
                );
            } break;
            default: {
                Global::print(
                    String("ERROR: Unsupported Variant argument Type: %")
                        .format(Array::make(argument_type), "%")
                );
                Global::print("Try convert it to a String or a PoolByteArray.");
                return false;
            }
        }
        if (error != SQLITE_OK) {
            Global::print(String("SQL Error: Failed to bind argument %: %")
                              .format(Array::make(index, error), "%"));
            return false;
        }
    }
    return true;
}

Dictionary parse_row(sqlite3_stmt* statement, const Result result_type) {
    Dictionary result;
    const int column_count = sqlite3_column_count(statement);
    for (int column = 0; column < column_count; column++) {
        auto name = String(sqlite3_column_name(statement, column));
        Variant value;
        switch (sqlite3_column_type(statement, column)) {
            case SQLITE_INTEGER: {
                value = Variant(sqlite3_column_int(statement, column));
            } break;
            case SQLITE_FLOAT: {
                value = Variant(sqlite3_column_double(statement, column));
            } break;
            case SQLITE_TEXT: {
                String string(
                    (const char*)sqlite3_column_text(statement, column)
                );
                value = Variant(string);
            } break;
            case SQLITE_BLOB: {
                PoolByteArray data;
                const int size = sqlite3_column_bytes(statement, column);
                data.resize(size);
                memcpy(
                    data.write().ptr(),
                    sqlite3_column_blob(statement, column),
                    size
                );
                value = Variant(data);
            } break;
            default:
                break;
        }
        switch (result_type) {
            case Result::NUM:
                result[column] = value;
            case Result::ASSOC:
                result[name] = value;
            case Result::BOTH:
                result[column] = value;
                result[name]   = value;
        }
    }
    return result;
}

Array fetch_rows(
    sqlite3* database,
    const String& query,
    const Array& args,
    const Result result_type
) {
    Array result;
    if (query.strip_edges().empty()) {
        return result;
    }
    sqlite3_stmt* statement =
        prepare(database, query.strip_edges().utf8().get_data());
    if (!statement) {
        return result;
    }
    if (!bind_args(statement, args)) {
        sqlite3_finalize(statement);
        return result;
    }

    while (sqlite3_step(statement) == SQLITE_ROW) {
        result.append(parse_row(statement, result_type));
    }
    sqlite3_finalize(statement);
    return result;
}
} // namespace

SQLite::~SQLite() {
    close();
}

bool SQLite::open(String path) {
    return open_with_flags(path, DEFAULT_OPEN_FLAGS);
}

bool SQLite::open_with_flags(String path, const int flags) {
    if (path.strip_edges().empty()) {
        return false;
    }

    // If not running in the editor, databases in res:// are packed.
    if (!Engine::get_singleton()->is_editor_hint()
        && path.begins_with_char_array("res://")) {
        Ref<File> db_file;
        db_file.instance();
        if (!db_file->file_exists(path)) {
            Global::print(String("ERROR: Database file does not exist: %")
                              .format(Array::make(path), "%"));
            return false;
        }
        if (db_file->open(path, File::READ) != Error::OK) {
            Global::print("ERROR: Cannot open packed database file!");
            return false;
        }
        const int64_t size       = db_file->get_len();
        const PoolByteArray data = db_file->get_buffer(size);
        return open_buffered_with_flags(path, data, size, flags);
    }

    int flag_test = flags & ~ALLOWED_FLAGS_MASK;
    if (flag_test != 0) {
        Global::print(String("WARNING: Bad flags (%) used in open_with_flags!")
                          .format(Array::make(flag_test), "%"));
    }

    const String global_path =
        ProjectSettings::get_singleton()->globalize_path(path.strip_edges());
    const int error = sqlite3_open_v2(
        global_path.utf8().get_data(),
        &db,
        flags & ALLOWED_FLAGS_MASK,
        nullptr
    );

    if (error != SQLITE_OK) {
        Global::print("ERROR: Cannot open database!");
        return false;
    }
    return true;
}

bool SQLite::open_buffered(
    String path,
    PoolByteArray data,
    const int64_t size
) {
    return open_buffered_with_flags(path, data, size, DEFAULT_OPEN_FLAGS);
}

bool SQLite::open_buffered_with_flags(
    String path,
    PoolByteArray data,
    const int64_t size,
    const int flags
) {
    if (path.strip_edges().empty() || size == 0 || data.size() == 0) {
        return false;
    }

    // Initialize memory buffer
    const auto buffer = (spmembuffer_t*)calloc(1, sizeof(spmembuffer_t));
    buffer->total     = size;
    buffer->used      = size;
    buffer->data      = (char*)malloc(size + 1);
    memcpy(buffer->data, data.read().ptr(), size);
    buffer->data[size] = '\0';

    // Open database
    spmemvfs_env_init();
    const int error =
        spmemvfs_open_db(&memory_db, path.utf8().get_data(), buffer, flags);
    if (error != SQLITE_OK || memory_db.mem != buffer) {
        Global::print("ERROR: Cannot open buffered database!");
        return false;
    }
    in_memory = true;
    return true;
}

bool SQLite::open_encrypted(String path, String password) {
    return open_encrypted_with_flags(path, password, DEFAULT_OPEN_FLAGS);
}

bool SQLite::open_encrypted_with_flags(
    String path,
    String password,
    const int flags
) {
    if (!open_with_flags(path, flags)) {
        return false;
    }

    const int error = sqlite3_key(
        get_database(),
        password.utf8().get_data(),
        password.length()
    );
    if (error != SQLITE_OK) {
        Global::print(
            "ERROR: Failed to decrypt database, with provided password!"
        );
        close();
        return false;
    }
    return true;
}

void SQLite::close() {
    if (db) {
        if (sqlite3_close_v2(db) != SQLITE_OK) {
            Global::print("WARNING: Failed to close the database!");
        } else {
            db = nullptr;
        }
    }

    if (in_memory) {
        spmemvfs_close_db(&memory_db);
        spmemvfs_env_fini();
        in_memory = false;
    }
}

bool SQLite::query(String query) const {
    return query_with_args(query, PoolStringArray());
}

bool SQLite::query_with_args(String query, Array args) const {
    sqlite3_stmt* statement = prepare(get_database(), query.utf8().get_data());
    if (!statement) {
        return false;
    }
    if (!bind_args(statement, args)) {
        sqlite3_finalize(statement);
        return false;
    }
    sqlite3_step(statement);
    sqlite3_finalize(statement);

    return true;
}

bool SQLite::query_all(String query_list) const {
    sqlite3* database = get_database();
    if (!database) {
        Global::print("ERROR: SQL query requires an open database.");
        return false;
    }

    const int error = sqlite3_exec(
        database,
        query_list.utf8().get_data(),
        nullptr,
        nullptr,
        nullptr
    );
    if (error != SQLITE_OK) {
        Global::print("SQL Error: " + String(sqlite3_errmsg(database)));
        return false;
    }

    return true;
}

Array SQLite::fetch_array(String query) const {
    return fetch_rows(get_database(), query, Array(), Result::BOTH);
}

Array SQLite::fetch_array_with_args(String query, Array args) const {
    return fetch_rows(get_database(), query, args, Result::BOTH);
}

Array SQLite::fetch_assoc(String query) const {
    return fetch_rows(get_database(), query, Array(), Result::ASSOC);
}

Array SQLite::fetch_assoc_with_args(String query, Array args) const {
    return fetch_rows(get_database(), query, args, Result::ASSOC);
}

void SQLite::register_methods() {
    // Method list
    register_method("open", &SQLite::open);
    register_method("open_with_flags", &SQLite::open_with_flags);
    register_method("open_buffered", &SQLite::open_buffered);
    register_method(
        "open_buffered_with_flags",
        &SQLite::open_buffered_with_flags
    );
    register_method("open_encrypted", &SQLite::open_encrypted);
    register_method(
        "open_encrypted_with_flags",
        &SQLite::open_encrypted_with_flags
    );
    register_method("close", &SQLite::close);
    register_method("query", &SQLite::query);
    register_method("query_with_args", &SQLite::query_with_args);
    register_method("query_all", &SQLite::query_all);
    register_method("fetch_array", &SQLite::fetch_array);
    register_method("fetch_array_with_args", &SQLite::fetch_array_with_args);
    register_method("fetch_assoc", &SQLite::fetch_assoc);
    register_method("fetch_assoc_with_args", &SQLite::fetch_assoc_with_args);
}
} // namespace Rebel
