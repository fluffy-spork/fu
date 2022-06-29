#pragma once

typedef sqlite3 db_t;

db_t * db = NULL;

#define log_error_db(db, label) \
    error_log(sqlite3_errmsg(db), label, sqlite3_extended_errcode(db))

#define assert_open_db(file, ppdb) assert(open_db(wrap_blob(file), ppdb) != -1)
#define assert_ddl_db(db, sql) assert(ddl_db(db, wrap_blob(sql)) != -1)
#define assert_prepare_db(db, ppstmt, sql) assert(prepare_db(db, ppstmt, wrap_blob(sql)) != -1)

typedef struct {
    sqlite3_stmt * stmt;
    param_t * params;
    u8 n_params;
} stmt_db_t;

enum {
    session_query_id_stmt,
} stmt_id_t;

int
open_db(blob_t * file, db_t ** db)
{
    assert(file != NULL);
    assert(file->size < 256);

    if (begins_with_blob(file, wrap_blob("/"))) {
        error_log("file must relative path", "file", 1);
        return -1;
    }

    if (sqlite3_open(cstr_blob(file), db)) {
        log_error_db(*db, "sqlite3_open");
        return -1;
    }

    return 0;
}

int
close_db(db_t * db)
{
    sqlite3_close(db);
    return 0;
}

int
ddl_db(db_t * db, blob_t * stmt)
{
    assert(db != NULL);

    // NOTE(jason): was using msg for last arg, but IDK if it's different from
    // sqlite3_errmsg.  does not appear to matter as a sql error came out as
    // expected.  cool.
    if (sqlite3_exec(db, cstr_blob(stmt), NULL, NULL, NULL)) {
        log_error_db(db, "sqlite3_exec");
        return -1;
    }

    return 0;
}

int
prepare_db(db_t * db, sqlite3_stmt **stmt, blob_t * sql)
{
    assert(db != NULL);
    assert(sql != NULL);

    if (sqlite3_prepare_v2(db, cstr_blob(sql), -1, stmt, NULL)) {
        log_error_db(db, "sqlite3_prepare_v2");
        return -1;
    }

    return 0;
}

int
finalize_db(sqlite3_stmt * stmt)
{
    int result = sqlite3_finalize(stmt);
    if (result) {
        log_error_db(db, "sqlite3_finalize");
    }

    return result;
}

// 1 based index
int
text_bind_db(sqlite3_stmt * stmt, int index, blob_t * value)
{
    return sqlite3_bind_text(stmt, index, (char *)value->data, value->size, SQLITE_STATIC);
}

// 1 based index
int
s64_bind_db(sqlite3_stmt * stmt, int index, s64 value)
{
    return sqlite3_bind_int64(stmt, index, value);
}

// 0 based index
s64
s64_db(sqlite3_stmt * stmt, int index)
{
    return sqlite3_column_int64(stmt,  index);
}

// 0 based index
ssize_t
blob_db(sqlite3_stmt * stmt, int index, blob_t * value)
{
    int n = sqlite3_column_bytes(stmt, index);
    if (n > 0) {
        return write_blob(value, sqlite3_column_text(stmt,  index), n);
    }

    return 0;
}

int
set_s64_pragma_db(db_t * db, blob_t * pragma, s64 value)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, wrap_blob("pragma "));
    add_blob(sql, pragma);
    write_blob(sql, "=", 1);
    add_s64_blob(sql, value);

    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, sql)) {
        return -1;
    }

    int result = sqlite3_step(stmt);
    if (result != SQLITE_DONE) {
        log_error_db(db, "sqlite3_step");
    }

    sqlite3_finalize(stmt);

    return result;
}

s64
s64_pragma_db(db_t * db, blob_t * pragma)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, wrap_blob("pragma "));
    add_blob(sql, pragma);

    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, sql)) {
        return -1;
    }

    s64 result = 0;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return result;
}

s32
application_id_db(db_t * db)
{
    return (s32)s64_pragma_db(db, wrap_blob("application_id"));
}

int
set_application_id_db(db_t * db, s32 id)
{
    return set_s64_pragma_db(db, wrap_blob("application_id"), id);
}

s32
user_version_db(db_t * db)
{
    return (s32)s64_pragma_db(db, wrap_blob("user_version"));
}

int
set_user_version_db(db_t * db, s32 version)
{
    return set_s64_pragma_db(db, wrap_blob("user_version"), version);
}

