#pragma once

typedef sqlite3 db_t;

db_t * db = NULL;

#define log_error_db(db, label) \
    error_log(sqlite3_errmsg(db), label, sqlite3_extended_errcode(db))

// these are a bad idea since asserts could be disabled and the code wouldn't
// be run at all.  oops!
// should make them something like abort_prepare_db or fail_prepare_db.  should
// work the same but always run the code and fail.
#define assert_open_db(file, ppdb) assert(open_db(wrap_blob(file), ppdb) != -1)
#define assert_ddl_db(db, sql) assert(ddl_db(db, wrap_blob(sql)) != -1)
#define assert_prepare_db(db, ppstmt, sql) assert(prepare_db(db, ppstmt, wrap_blob(sql)) != -1)

void
log_callback_db(void * arg, int err_code, const char *msg)
{
    UNUSED(arg);

    error_log(msg, "sqlite", err_code)
}

// NOTE(jason): currently not the end of the world if this isn't called, but
// the log callback won't be setup which is helpful
int
init_db()
{
    // NOTE(jason): has to be called before sqlite3_initialize
    sqlite3_config(SQLITE_CONFIG_LOG, log_callback_db, NULL);
    return sqlite3_initialize();
}

int
open_db(blob_t * file, db_t ** db)
{
    assert(file != NULL);
    assert(file->size < 256);

    if (begins_with_blob(file, B("/"))) {
        error_log("file must relative path", "file", 1);
        return -1;
    }

    if (sqlite3_open(cstr_blob(file), db)) {
        log_error_db(*db, "sqlite3_open");
        return -1;
    }

    // NOTE(jason): somewhat shockingly, there isn't a default timeout.  this
    // caused a bunch of wasted time trying to figure out a concurrency issue
    // that was 3 inserts.  mistaking SQLITE_LOCKED for SQLITE_BUSY didn't
    // help:(  The message is "database is locked" for SQLITE_BUSY which seems
    // misleading.  1 second easily fixes the problem.
    // the time is in "milliseconds" not seconds as I initially thought.
    sqlite3_busy_timeout(*db, 1000);

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
prepare_db(db_t * db, sqlite3_stmt **stmt, const blob_t * sql)
{
    assert(db != NULL);
    assert(sql != NULL);

    //debug_blob(sql);

    if (sqlite3_prepare_v2(db, cstr_blob(sql), -1, stmt, NULL)) {
        log_error_db(db, "sqlite3_prepare_v2");
        return -1;
    }

    return 0;
}

//#define finalize_db(stmt) _finalize_db(stmt, __FILE__, __LINE__)
//_finalize_db(sqlite3_stmt * stmt, const char * file, s64 line)

int
finalize_db(sqlite3_stmt * stmt)
{
    int result = sqlite3_finalize(stmt);
    if (result) {
        //debugf("finalize failed: %ld: %s", line, file);
        log_error_db(db, "sqlite3_finalize");
    }

    return result;
}

int
begin_db(db_t * db)
{
    return ddl_db(db, B("begin"));
}

int
commit_db(db_t * db)
{
    return ddl_db(db, B("commit"));
}

int
rollback_db(db_t * db)
{
    // NOTE(jason): not in a transaction
    if (sqlite3_get_autocommit(db)) return 0;

    return ddl_db(db, B("rollback"));
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
blob_pragma_db(db_t * db, const blob_t * pragma, blob_t * value)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, B("pragma "));
    add_blob(sql, pragma);

    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, sql)) {
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    return sqlite3_finalize(stmt);
}

int
set_blob_pragma_db(db_t * db, const blob_t * pragma, const blob_t * value)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, B("pragma "));
    add_blob(sql, pragma);
    write_blob(sql, "=", 1);
    add_blob(sql, value);

    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, sql)) {
        return -1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        log_error_db(db, "sqlite3_step");
    }

    return sqlite3_finalize(stmt);
}

int
set_s64_pragma_db(db_t * db, blob_t * pragma, s64 value)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, B("pragma "));
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

    add_blob(sql, B("pragma "));
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
    return (s32)s64_pragma_db(db, B("application_id"));
}

int
set_application_id_db(db_t * db, s32 id)
{
    return set_s64_pragma_db(db, B("application_id"), id);
}

s32
user_version_db(db_t * db)
{
    return (s32)s64_pragma_db(db, B("user_version"));
}

int
set_user_version_db(db_t * db, s32 version)
{
    return set_s64_pragma_db(db, B("user_version"), version);
}

int
set_wal_mode_db(db_t * db)
{
    return set_blob_pragma_db(db, B("journal_mode"), B("wal"));
}

int
exec_s64_db(db_t * db, blob_t * sql, s64 * value)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *value = s64_db(stmt, 0);
    }

    result = finalize_db(stmt);

    return result;
}

int
exec_s64_1p_db(db_t * db, blob_t * sql, s64 * value, blob_t * p1)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = text_bind_db(stmt, 1, p1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *value = s64_db(stmt, 0);
    }

    result = finalize_db(stmt);

    return result;
}

// NOTE(jason): all parameters should be text that sqlite will convert or app
// converts to text first.  at least that's the theory.
int
exec_s64_2p_db(db_t * db, blob_t * sql, s64 * value, blob_t * p1, blob_t * p2)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = text_bind_db(stmt, 1, p1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    result = text_bind_db(stmt, 2, p2);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *value = s64_db(stmt, 0);
    }

    result = finalize_db(stmt);

    return result;
}

int
exec_s64_pii_db(db_t * db, blob_t * sql, s64 * value, s64 p1, s64 p2)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = s64_bind_db(stmt, 1, p1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    result = s64_bind_db(stmt, 2, p2);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *value = s64_db(stmt, 0);
    }

    result = finalize_db(stmt);

    return result;
}

// TODO(jason): should this return an error like SQLITE_NOTFOUND if a value
// isn't found?  currently have to check for the value being empty
int
exec_blob_db(db_t * db, blob_t * sql, blob_t * value)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    result = finalize_db(stmt);

    return result;
}

int
exec_blob_pb_db(db_t * db, blob_t * sql, blob_t * value, blob_t * p1)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = text_bind_db(stmt, 1, p1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    result = finalize_db(stmt);

    return result;
}

int
exec_blob_pi_db(db_t * db, blob_t * sql, blob_t * value, s64 p1)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = s64_bind_db(stmt, 1, p1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    result = finalize_db(stmt);

    return result;
}

int
exec_blob_2b_db(db_t * db, blob_t * sql, blob_t * value, blob_t * p1, blob_t * p2)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = text_bind_db(stmt, 1, p1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    result = text_bind_db(stmt, 2, p2);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    result = finalize_db(stmt);

    return result;
}

int
exec_blob_i1b2_db(db_t * db, blob_t * sql, blob_t * value, s64 i1, blob_t * b1, blob_t * b2)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = s64_bind_db(stmt, 1, i1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    result = text_bind_db(stmt, 2, b1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    result = text_bind_db(stmt, 3, b2);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    result = finalize_db(stmt);

    return result;
}

int
insert_params(db_t * db, blob_t * table, s64 * id, s64 user_id, param_t * params, int n_params)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, B("insert into "));
    add_blob(sql, table);

    add_blob(sql, B(" ("));
    add_blob(sql, fields.user_id->name);
    for (int i = 0; i < n_params; i++) {
        write_blob(sql, ",", 1);
        add_blob(sql, params[i].field->name);
    }
    add_blob(sql, B(") values (?"));
    add_s64_blob(sql, fields.user_id->id);
    for (int i = 0; i < n_params; i++) {
        write_blob(sql, ",?", 2);
        add_s64_blob(sql, params[i].field->id);
    }
    add_blob(sql, B(") returning "));
    add_blob(sql, table);
    add_blob(sql, B("_id"));

    log_var_blob(sql);

    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = s64_bind_db(stmt, fields.user_id->id, user_id);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        result = text_bind_db(stmt, p->field->id, p->value);
        if (result != SQLITE_OK) {
            finalize_db(stmt);
            return result;
        }
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *id = s64_db(stmt, 0);
    }

    result = finalize_db(stmt);

    return result;
}

int
update_params(db_t * db, blob_t * table, param_t * params, int n_params, field_t * id_field, s64 id)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, B("update "));
    add_blob(sql, table);

    add_blob(sql, B(" set "));
    add_blob(sql, fields.modified->name);
    add_blob(sql, B("=unixepoch()"));

    for (int i = 0; i < n_params; i++) {
        write_blob(sql, ",", 1);
        add_blob(sql, params[i].field->name);
        write_blob(sql, "=?", 2);
        add_s64_blob(sql, params[i].field->id);
    }

    add_blob(sql, B(" where "));
    add_blob(sql, id_field->name);
    write_blob(sql, "=?", 2);
    add_s64_blob(sql, id_field->id);

    add_blob(sql, B(" returning "));
    add_blob(sql, id_field->name);

    log_var_blob(sql);

    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = s64_bind_db(stmt, id_field->id, id);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        result = text_bind_db(stmt, p->field->id, p->value);
        if (result != SQLITE_OK) {
            finalize_db(stmt);
            return result;
        }
    }

    s64 returning_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        returning_id = s64_db(stmt, 0);
    }

    if (returning_id != id) {
        finalize_db(stmt);
        result = -1;
    }
    else {
        result = finalize_db(stmt);
    }

    return result;
}

// TODO(jason): this should possibly be returning the number of rows deleted,
// but currently only used for deleting one row.  so the returning clause would
// be removed.
int
delete_params(db_t * db, blob_t * table, param_t * params, int n_params, field_t * id_field, s64 id)
{
    blob_t * sql = tmp_blob();

    add_blob(sql, B("delete from "));
    add_blob(sql, table);

    add_blob(sql, B(" where "));
    add_blob(sql, id_field->name);
    write_blob(sql, "=?", 2);
    add_s64_blob(sql, id_field->id);

    for (int i = 0; i < n_params; i++) {
        write_blob(sql, " and ", 5);
        add_blob(sql, params[i].field->name);
        write_blob(sql, "=?", 2);
        add_s64_blob(sql, params[i].field->id);
    }

    add_blob(sql, B(" returning "));
    add_blob(sql, id_field->name);

    log_var_blob(sql);

    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = s64_bind_db(stmt, id_field->id, id);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        result = text_bind_db(stmt, p->field->id, p->value);
        if (result != SQLITE_OK) {
            finalize_db(stmt);
            return result;
        }
    }

    s64 returning_id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        returning_id = s64_db(stmt, 0);
    }

    if (returning_id != id) {
        finalize_db(stmt);
        result = -1;
    }
    else {
        result = finalize_db(stmt);
    }

    return result;
}

typedef struct row_handler_db_s row_handler_db_t;

struct row_handler_db_s {
    int (* func)(row_handler_db_t * handler);
    sqlite3_stmt * stmt;
    s64 n_rows;
    s64 last_id;
    void * data;
};

// run the query and calling callback for each row.  If there are no rows, the
// callback is called with a NULL sqlite3_stmt
int
rows_pii_db(db_t * db, const blob_t * sql, row_handler_db_t * handler, s64 p1, s64 p2)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    result = s64_bind_db(stmt, 1, p1);
    if (result != SQLITE_OK) {
        finalize_db(stmt);
        return result;
    }

    if (sqlite3_bind_parameter_count(stmt) > 1) {
        result = s64_bind_db(stmt, 2, p2);
        if (result != SQLITE_OK) {
            finalize_db(stmt);
            return result;
        }
    }

    handler->n_rows = 0;
    handler->stmt = stmt;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        handler->func(handler);
        handler->n_rows++;
    }

    handler->stmt = NULL;
    result = finalize_db(stmt);

    return result;
}

int
rows_params_db(db_t * db, const blob_t * sql, row_handler_db_t * handler, param_t * params, int n_params)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        result = text_bind_db(stmt, p->field->id, p->value);
        if (result != SQLITE_OK) {
            finalize_db(stmt);
            return result;
        }
    }

    handler->n_rows = 0;
    handler->stmt = stmt;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        handler->func(handler);
        handler->n_rows++;
    }

    handler->stmt = NULL;
    result = finalize_db(stmt);

    return result;
}

