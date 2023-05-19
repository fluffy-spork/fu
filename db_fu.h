#pragma once

#include "file_fu.h"

typedef sqlite3 db_t;

#define log_error_db(db, label) \
    error_log(sqlite3_errmsg(db), label, sqlite3_extended_errcode(db))
#define log_error_stmt_db(stmt, label) \
    log_error_db(sqlite3_db_handle(stmt), label)


// these are a bad idea since asserts could be disabled and the code wouldn't
// be run at all.  oops!
// should make them something like abort_prepare_db or fail_prepare_db.  should
// work the same but always run the code and fail.
#define assert_open_db(file, ppdb) assert(open_db(B(file), ppdb) != -1)
#define assert_ddl_db(db, sql) assert(ddl_db(db, B(sql)) != -1)
#define assert_prepare_db(db, ppstmt, sql) assert(prepare_db(db, ppstmt, B(sql)) != -1)

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

#define MAX_BUSY_TIMEOUT_MS_DB 3000

// delays a random number of microseconds between 0 and 1s
int
busy_handler_db(void * ptr, int count)
{
    UNUSED(ptr);

    if (count > 5) return 0;

    // one_us = 1000000;
    s64 delay_us = random_range_s64_fu(0, 1000000);
    //debug_s64(delay_us);
    if (delay_us) {
        usleep(delay_us);
        return 1;
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
        log_var_blob(sql);
        log_error_db(db, "sqlite3_prepare_v2");
        return -1;
    }

    return 0;
}

int
prepare_file_db(db_t * db, sqlite3_stmt **stmt, const blob_t * path)
{
    blob_t * sql = local_blob(8*1024);

    if (load_file(path, sql)) {
        return log_errno("load_file");
    }

    return prepare_db(db, stmt, sql);
}

//#define finalize_db(stmt) _finalize_db(stmt, __FILE__, __LINE__)
//_finalize_db(sqlite3_stmt * stmt, const char * file, s64 line)

// TODO(jason): the way i'm using this isn't quite right as a bind error will
// still return 0 for sqlite3_finalize since the statement hasn't been
// executed.  I want it to print an error if any error has happened since the
// last finalize.  possibly should change the name.
int
finalize_db(sqlite3_stmt * stmt)
{
    int rc = sqlite3_finalize(stmt);
    if (rc) {
        log_error_stmt_db(stmt, sqlite3_sql(stmt));
    }

    return rc;
}

bool
in_txn_db(db_t * db)
{
    return !sqlite3_get_autocommit(db);
}

int
begin_db(db_t * db)
{
    return ddl_db(db, B("begin"));
}

// use for a db function that's meant to be called by other functions and needs
// to be in a transaction.  tried using savepoints as nested transactions, but
// that created problems when not called with a transaction.  in most cases
// nested transactions are probably a bad idea.  most cases should be all or
// nothing anyway and don't need parts of the transaction to commit.
int
require_txn_db(db_t * db)
{
    if (in_txn_db(db)) return 0;

    dev_error("transaction required");
}

int
commit_db(db_t * db)
{
    return ddl_db(db, B("commit"));
}

// TODO(jason): rollback_dbl was assumed to return non-zero as it's only called after an error
// doesn't seem to matter if the rollback succeeded
// not sure how this should work but needs to work in the case of || multiple statements in
// a transaction
int
rollback_db(db_t * db)
{
    if (!in_txn_db(db)) return 0;

    return ddl_db(db, B("rollback"));
}

// TODO(jason): i think for my usage, this shouldn't be used and should just
// have ensure_transaction or something that will only start a new transaction
// if there isn't one instead of nested transactions.
//
// NOTE(jason): this just doesn't work.  rollback of the last savepoint doesn't
// rollback the auto transaction.  seems kind of dumb, but I don't think I
// understand the point of savepoints and nested transactions
/*
int
savepoint_db(db_t * db, const blob_t * name)
{
    blob_t * sql = local_blob(256);
    vadd_blob(sql, B("savepoint "), name);
    return ddl_db(db, sql);
}

int
release_db(db_t * db, const blob_t * name)
{
    blob_t * sql = local_blob(256);
    vadd_blob(sql, B("release "), name);
    return ddl_db(db, sql);
}

int
rollback_to_db(db_t * db, const blob_t * name)
{
    blob_t * sql = local_blob(256);
    vadd_blob(sql, B("rollback to "), name);
    return ddl_db(db, sql);
}
*/

// 1 based index
int
cstr_bind_db(sqlite3_stmt * stmt, int index, const char * value)
{
    int len = value[0] ? strlen(value) : 0;
    int rc = 0;
    if (len) {
        rc = sqlite3_bind_text(stmt, index, value, len, SQLITE_STATIC);
    }
    else {
        rc = sqlite3_bind_null(stmt, index);
    }

    if (rc) {
        log_error_stmt_db(stmt, "cstr bind failed");
        return -1;
    }

    return 0;
}

// 1 based index
int
text_bind_db(sqlite3_stmt * stmt, int index, const blob_t * value)
{
    int rc = sqlite3_bind_text(stmt, index, (char *)value->data, value->size, SQLITE_STATIC);
    if (rc) {
        log_error_stmt_db(stmt, "text bind failed");
        return -1;
    }

    return 0;
}

// 1 based index
int
s64_bind_db(sqlite3_stmt * stmt, int index, s64 value)
{
    int rc = sqlite3_bind_int64(stmt, index, value);
    if (rc) {
        log_error_stmt_db(stmt, "s64 bind failed");
        return -1;
    }

    return 0;
}

// 0 based index
s64
s64_db(sqlite3_stmt * stmt, int index)
{
    return sqlite3_column_int64(stmt,  index);
}

// 0 based index
// should be text_db since getting a blob could be different?
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
    blob_t * sql = local_blob(255);

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
    blob_t * sql = local_blob(255);

    add_blob(sql, B("pragma "));
    add_blob(sql, pragma);
    write_blob(sql, "=", 1);
    add_blob(sql, value);

    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, sql)) {
        return -1;
    }

    // NOTE(jason): this returns a row unlike s64 case for user_version.  at
    // least for locking_mode and journal_mode.
    // TODO(jason): should this be checking the value is the set value?
    if (sqlite3_step(stmt) != SQLITE_ROW) {
        log_error_db(db, "sqlite3_step");
    }

    return sqlite3_finalize(stmt);
}

int
set_s64_pragma_db(db_t * db, blob_t * pragma, s64 value)
{
    blob_t * sql = local_blob(255);

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
    blob_t * sql = local_blob(255);

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
set_user_version_db(db_t * db, s32 user_version)
{
    log_var_s64(user_version);
    return set_s64_pragma_db(db, B("user_version"), user_version);
}

int
set_wal_mode_db(db_t * db)
{
    return set_blob_pragma_db(db, B("journal_mode"), B("wal"));
}

int
set_exclusive_locking_mode_db(db_t * db)
{
    return set_blob_pragma_db(db, B("locking_mode"), B("exclusive"));
}

int
open_db(const blob_t * file, db_t ** db)
{
    assert_not_null(file);
    assert(file->size < 256);

    if (sqlite3_open(cstr_blob(file), db)) {
        log_error_db(*db, "sqlite3_open");
        return -1;
    }

    sqlite3_extended_result_codes(*db, 1);

    // NOTE(jason): somewhat shockingly, there isn't a default timeout.  this
    // caused a bunch of wasted time trying to figure out a concurrency issue
    // that was 3 inserts.  mistaking SQLITE_LOCKED for SQLITE_BUSY didn't
    // help:(  The message is "database is locked" for SQLITE_BUSY which seems
    // misleading.  1 second easily fixes the problem.
    // the time is in "milliseconds" not seconds as I initially thought.
    //sqlite3_busy_timeout(*db, 3000);
    // NOTE(jason): this avoids the thundering herd problem with multiple
    // processes on shutdown and probably other cases.  Just does a random
    // sleep in the range as the sqlite default is deterministic and all
    // processes do the same delay algo.  Seems odd this is the default
    // assuming this is all correct.
    sqlite3_busy_handler(*db, busy_handler_db, NULL);

    // NOTE(jason): does anyone use anything other than wal mode?  With a web
    // server this seems like the only way to go.
    if (set_wal_mode_db(*db)) {
        log_error_db(*db, "failed to set wal mode");
    }

    return 0;
}

int
exec_s64_db(db_t * db, blob_t * sql, s64 * value)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)) return -1;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (value) *value = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

int
exec_s64_pb_db(db_t * db, blob_t * sql, s64 * value, const blob_t * p1)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || text_bind_db(stmt, 1, p1)
       )
    {
        finalize_db(stmt);
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (value) *value = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

// NOTE(jason): all parameters should be text that sqlite will convert or app
// converts to text first.  at least that's the theory.
int
exec_s64_pbb_db(db_t * db, blob_t * sql, s64 * value, blob_t * p1, blob_t * p2)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || text_bind_db(stmt, 1, p1)
            || text_bind_db(stmt, 2, p2))
    {
        return finalize_db(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (value) *value = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

int
exec_s64_pbi_db(db_t * db, const blob_t * sql, s64 * value, const blob_t * p1, const s64 p2)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || text_bind_db(stmt, 1, p1)
            || s64_bind_db(stmt, 2, p2))
    {
        int rc = sqlite3_errcode(db);
        if (stmt) {
            finalize_db(stmt);
        }

        return rc;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (value) *value = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

int
exec_s64_pi_db(db_t * db, blob_t * sql, s64 * value, s64 p1)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || s64_bind_db(stmt, 1, p1))
    {
        return finalize_db(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (value) *value = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

int
exec_s64_pii_db(db_t * db, const blob_t * sql, s64 * value, s64 p1, s64 p2)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || s64_bind_db(stmt, 1, p1)
            || s64_bind_db(stmt, 2, p2))
    {
        return finalize_db(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (value) *value = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

// TODO(jason): should this return an error like SQLITE_NOTFOUND if a value
// isn't found?  currently have to check for the value being empty
int
exec_blob_db(db_t * db, blob_t * sql, blob_t * value)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)) return -1;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    return finalize_db(stmt);
}

int
exec_blob_pb_db(db_t * db, blob_t * sql, blob_t * value, const blob_t * p1)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || text_bind_db(stmt, 1, p1))
    {
        return finalize_db(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    return finalize_db(stmt);
}

int
exec_blob_pi_db(db_t * db, blob_t * sql, blob_t * value, s64 p1)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || s64_bind_db(stmt, 1, p1))
    {
        return finalize_db(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    return finalize_db(stmt);
}

int
exec_blob_pbb_db(db_t * db, blob_t * sql, blob_t * value, const blob_t * p1, const blob_t * p2)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || text_bind_db(stmt, 1, p1)
            || text_bind_db(stmt, 2, p2))
    {
        return finalize_db(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    return finalize_db(stmt);
}

int
exec_blob_pibb_db(db_t * db, blob_t * sql, blob_t * value, s64 i1, blob_t * b1, blob_t * b2)
{
    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || s64_bind_db(stmt, 1, i1)
            || text_bind_db(stmt, 2, b1)
            || text_bind_db(stmt, 3, b2))
    {
        return finalize_db(stmt);
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        blob_db(stmt, 0, value);
    }

    return finalize_db(stmt);
}

// NOTE(jason): automatically adds a created field with unixepoch() value
int
insert_sql_db(blob_t * sql, const blob_t * table, param_t * params, int n_params)
{
    add_blob(sql, B("insert into "));
    add_blob(sql, table);

    add_blob(sql, B(" ("));
    add_blob(sql, fields.created->name);
    for (int i = 0; i < n_params; i++) {
        write_blob(sql, ",", 1);
        add_blob(sql, params[i].field->name);
    }
    add_blob(sql, B(") values ("));
    add_blob(sql, B("unixepoch()"));
    for (int i = 0; i < n_params; i++) {
        write_blob(sql, ",?", 2);
        add_s64_blob(sql, params[i].field->id);
    }
    add_blob(sql, B(")"));

    return sql->error ? -1 : 0;
}

#define insert_fields_db(db, ...) _insert_fields_db(db, __VA_ARGS__, 0)

// NOTE(jason): probably won't work with without rowid tables due to
// "returning rowid" clause
int
_insert_fields_db(db_t * db, const blob_t * table, s64 * rowid, ...)
{
    va_list args;
    va_start(args, rowid);

    // NOTE(jason): can't do N_FIELDS for params size because then the
    // loop to bind params, etc has to loop over all fields to check
    // for actually provided values or have a lookup or something.
    // eventually this should be removed when all db stuff is inited
    // at startup.
    const int max_params = 16;
    param_t params[max_params];
    int n_params = 0;

    /// how to detect too many varargs for max_params?
    for (int i = 0; i < max_params; i++) {
        field_id_t id = va_arg(args, s64);
        if (!id) break;

        field_t * f = by_id_field(id);
        params[i] = stk_param(f);

        //log_var_field(params[i].field);
        //debug_s64(params[i].value->capacity);

        switch (f->type) {
            case text_field_type:
                {
                    blob_t * value = va_arg(args, blob_t *);
                    //debug_blob(value);
                    dev_error(value == NULL);
                    add_blob(params[i].value, value);
                }
                break;
            case integer_field_type:
                {
                    s64 s64_value = va_arg(args, s64);
                    //debug_s64(s64_value);
                    add_s64_blob(params[i].value, s64_value);
                }
                break;
            default:
                log_var_field(f);
                dev_error("unhandled field type");
        }

        n_params++;
    }

    // TODO(jason): somehow test for extra params over the max_params

    blob_t * sql = stk_blob(4096);
    insert_sql_db(sql, table, params, n_params);
    add_blob(sql, B(" returning rowid"));

    //debug_blob(sql);

    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)) {
        return -1;
    }

    for (int i = 0; i < n_params; i++) {
        param_t p = params[i];
        if (text_bind_db(stmt, p.field->id, p.value)) {
            return finalize_db(stmt);
        }
    }

    if (sqlite3_step(stmt) == SQLITE_ROW && rowid) {
        *rowid = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

int
insert_params(db_t * db, const blob_t * table, s64 * id, s64 user_id, param_t * params, int n_params, field_id_t ignore_field)
{
    blob_t * sql = local_blob(255);

    add_blob(sql, B("insert into "));
    add_blob(sql, table);

    add_blob(sql, B(" ("));
    add_blob(sql, fields.user_id->name);
    for (int i = 0; i < n_params; i++) {
        if (params[i].field->id == ignore_field) continue;

        write_blob(sql, ",", 1);
        add_blob(sql, params[i].field->name);
    }
    add_blob(sql, B(") values (?"));
    add_s64_blob(sql, fields.user_id->id);
    for (int i = 0; i < n_params; i++) {
        if (params[i].field->id == ignore_field) continue;

        write_blob(sql, ",?", 2);
        add_s64_blob(sql, params[i].field->id);
    }
    add_blob(sql, B(") returning "));
    add_blob(sql, table);
    add_blob(sql, B("_id"));

    if (sql->error) {
        error_log_blob(sql);
        return -1;
    }

    //log_var_blob(sql);

    sqlite3_stmt * stmt;

    if (prepare_db(db, &stmt, sql)
            || s64_bind_db(stmt, fields.user_id->id, user_id))
    {
        return finalize_db(stmt);
    }

    for (int i = 0; i < n_params; i++) {
        if (params[i].field->id == ignore_field) continue;

        param_t * p = &params[i];
        if (text_bind_db(stmt, p->field->id, p->value)) {
            return finalize_db(stmt);
        }
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (id) *id = s64_db(stmt, 0);
    }

    return finalize_db(stmt);
}

int
update_params(db_t * db, blob_t * table, param_t * params, int n_params, field_t * id_field, s64 id, field_id_t ignore_field)
{
    blob_t * sql = local_blob(255);

    add_blob(sql, B("update "));
    add_blob(sql, table);

    add_blob(sql, B(" set "));
    add_blob(sql, fields.modified->name);
    add_blob(sql, B("=unixepoch()"));

    for (int i = 0; i < n_params; i++) {
        if (params[i].field->id == ignore_field) continue;

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

    //log_var_blob(sql);

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
        if (params[i].field->id == ignore_field) continue;

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
    blob_t * sql = local_blob(255);

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
    s64 n_rows;

    // TODO(jason): getting kind of wonky with these extra values to use
    // outside the callback.  probably should be including it all in data instead
    s64 last_id;
    s64 last_user_id;

    // TODO(jason): should outputs be passed as parameters to the row_handler?
    param_t * outputs;
    int n_outputs;
    void * data;
};

// NOTE(jason): sql should use field->id for bind parameters with "?12", etc
int
rows_params_db(db_t * db, const blob_t * sql, row_handler_db_t * handler, param_t * params, int n_params)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    for (int i = 0; i < n_params; i++) {
        param_t * p = &params[i];
        //debug_s64(p->field->id);
        result = text_bind_db(stmt, p->field->id, p->value);
        if (result != SQLITE_OK) {
            finalize_db(stmt);
            return result;
        }
    }

    int n_outputs = sqlite3_column_count(stmt);
    param_t outputs[n_outputs];
    for (int i = 0; i < n_outputs; i++) {
        param_t * p = &outputs[i];
        const char * column = sqlite3_column_name(stmt, i);
        field_t * f = by_name_field(B(column));
        if (!f) {
            debugf("unknown field name: %s", column);
            continue;
        }
        //debug_blob(f->name);

        // think this should work, but can't test now and this entire function
        // should eventually be deleted
        //outputs[i] = local_param(f);
        p->field = f;
        p->value = local_blob(f->max_size);
        p->error = local_blob(128);
    }

    handler->outputs = outputs;
    handler->n_outputs = n_outputs;
    handler->n_rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int i = 0; i < n_outputs; i++) {
            reset_blob(outputs[i].value);
            blob_db(stmt, i, outputs[i].value);
        }

        handler->func(handler);
        handler->n_rows++;
    }

    result = finalize_db(stmt);

    handler->outputs = NULL;
    handler->n_outputs = 0;

    return result;
}

int
rows_db(db_t * db, const blob_t * sql, row_handler_db_t * handler)
{
    return rows_params_db(db, sql, handler, NULL, 0);
}

int
sql_type_param(param_t * p)
{
    switch (p->field->type) {
        case integer_field_type:
        case timestamp_field_type:
        case select_integer_field_type:
            return SQLITE_INTEGER;
        default:
            return SQLITE_TEXT;
    }
}

int
bind_params_db(sqlite3_stmt * stmt, param_t * params, int n_params)
{
    int n_bind = sqlite3_bind_parameter_count(stmt);

    // NOTE(jason): i think the reason for bind_to_input was
    // to bind the params in order?  not sure.  eventually
    // bind_to_input would be done at program load and then saved
    // for all calls.  maybe that was why.
    int bind_to_param[n_bind];
    memset(bind_to_param, -1, sizeof(int) * n_bind);

    for (int i = 0; i < n_bind; i++) {
        const char * name = sqlite3_bind_parameter_name(stmt, i + 1);
        if (!name) {
            debugf("unamed bind param at index %d", i);
            dev_error("unamed bind param");
        }

        blob_t * bind_name = B(name + 1);

        for (int j = 0; j < n_params; j++) {
            param_t * p = &params[j];
            if (equal_blob(p->field->name, bind_name)) {
                bind_to_param[i] = j;
                break;
            }
        }

        // NOTE(jason): this should only happen if there's a developer error
        if (bind_to_param[i] == -1) {
            debugf("missing param for bind param %s", name);
            dev_error("missing param");
        }
    }

    // TODO(jason): cache the stuff above

    for (int i = 0; i < n_bind; i++) {
        int rc = 0;

        param_t * p = &params[bind_to_param[i]];
        int sql_type = sql_type_param(p);
        if (sql_type == SQLITE_INTEGER) {
            rc = s64_bind_db(stmt, i + 1, s64_blob(p->value, 0));
        }
        else {
            rc = text_bind_db(stmt, i + 1, p->value);
        }

        if (rc != SQLITE_OK) {
            // TODO(jason): should these be doing this still?
            return finalize_db(stmt);
        }
    }

    return 0;
}

int
ids_params_db(db_t * db, const blob_t * sql, s64 * ids, int * n_ids, param_t * inputs, int n_inputs)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    if (bind_params_db(stmt, inputs, n_inputs)) {
        finalize_db(stmt);
        return -1;
    }

    int max_ids = *n_ids;
    int i = 0;
    for (; i < max_ids; i++) {
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            ids[i] = 0;
            break;
        }

        ids[i] = s64_db(stmt, 0);
    }
    *n_ids = i;

    return finalize_db(stmt);
}

int
rows_ids_db(db_t * db, const blob_t * sql, row_handler_db_t * handler, s64 * ids, int n_ids)
{
    int result;
    sqlite3_stmt * stmt;

    result = prepare_db(db, &stmt, sql);
    if (result != SQLITE_OK) return result;

    int n_outputs = sqlite3_column_count(stmt);
    param_t outputs[n_outputs];
    for (int i = 0; i < n_outputs; i++) {
        param_t * p = &outputs[i];
        const char * column = sqlite3_column_name(stmt, i);
        field_t * f = by_name_field(B(column));
        if (!f) {
            debugf("unknown field name: %s", column);
            continue;
        }
        //debug_blob(f->name);

        p->field = f;
        p->value = local_blob(f->max_size);
        p->error = local_blob(128);
    }

    // TODO(jason): could also pass the ids array to a single statement using
    // carray.  it might not be faster though and requires a sqlite extension.
    handler->outputs = outputs;
    handler->n_outputs = n_outputs;
    handler->n_rows = 0;
    for (int i = 0; i < n_ids; i++) {
        sqlite3_reset(stmt);
        s64_bind_db(stmt, 1, ids[i]);
        if (sqlite3_step(stmt) != SQLITE_ROW) {
            // TODO(jason): maybe this should keep going as long as no error
            // even if there aren't any rows
            log_error_db(db, "rows_ids_db");
            break;
        }

        for (int i = 0; i < n_outputs; i++) {
            reset_blob(outputs[i].value);
            blob_db(stmt, i, outputs[i].value);
        }

        handler->func(handler);
        handler->n_rows++;
    }

    handler->outputs = NULL;
    handler->n_outputs = 0;

    return finalize_db(stmt);
}

#define by_id_db(db, ...) _by_id_db(db, __VA_ARGS__, NULL)

int
_by_id_db(db_t * db, const blob_t * sql, s64 id, ...)
{
    assert(id > 0);

    //debug_s64(id);
    //debug_blob(sql);

    sqlite3_stmt * stmt;
    if (prepare_db(db, &stmt, sql)) {
        return -1;
    }

    int n_parameters = sqlite3_bind_parameter_count(stmt);
    if (n_parameters != 1) {
        info_log("sql must have 1 bind parameter for the id");
        return -1;
    }

    s64_bind_db(stmt, 1, id);

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        // TODO(jason): probably should return an error value for no rows since
        // in this case there should always be 1 row
        info_log("sql exec failed");
        return -1;
    }

    int n_columns = sqlite3_column_count(stmt);
    if (n_columns < 1) {
        info_log("must have at least 1 output column");
        return -1;
    }

    va_list args;
    va_start(args, id);
    for (int i = 0; i < n_columns; i++) {
        param_t * p = va_arg(args, param_t *);
        if (!p) {
            info_log("not enough arguments");
            break;
        }

        reset_blob(p->value);
        blob_db(stmt, i, p->value);
    }
    va_end(args);

    return finalize_db(stmt);
}


#define by_id_fields_db(db, ...) _by_id_fields_db(db, __VA_ARGS__, 0)

int
_by_id_fields_db(db_t * db, const blob_t * table, s64 rowid, ...)
{
    UNUSED(db);
    UNUSED(table);
    UNUSED(rowid);

    return 0;
}


/*

char *
sql[255] = {
    "feed_select",
    "post_by_id_select"
};

int
select_db(int sql_id, s64 id, ...)
{
    return -1;
}

int
insert_db(int sql_id, s64 * id, ...)
{
    return -1;
}

int
update_db(int sql_id, s64 id, ...)
{
    return -1;
}

int
delete_db(int sql_id, s64 id)
{
    return -1;
}
*/

int
create_db_version_table_db(db_t * db)
{
    return ddl_db(db, B("create table if not exists db_version (src_file text primary key, version integer not null default 0) without rowid, strict"));
}

int
version_db(db_t * db, const blob_t * src_file, s64 * version)
{
    return exec_s64_pb_db(db, B("select version from db_version where src_file = ?1"), version, src_file);
}

int
set_version_db(db_t * db, const blob_t * src_file, s64 version)
{
    return exec_s64_pbi_db(db, B("insert or replace into db_version (src_file, version) values (?1, ?2) returning version"), NULL, src_file, version);
}

typedef struct {
    int version;
    char * sql;
} version_sql_t;

#define upgrade_db(file, versions) _upgrade_db(file, B(__FILE__), versions, array_size_fu(versions))

int
_upgrade_db(const blob_t * db_file, const blob_t * src_file, version_sql_t * versions, int n_versions)
{
    //debug_blob(src_file);
    //debug_blob(db_file);

    db_t * db;

    if (open_db(db_file, &db)) {
        return -1;
    }

    int rc = 0;
    s64 version = 0;

    // NOTE(jason): exclusive lock on whole db.  However, upgrades should
    // normally happen in main before other process/threads are created.
    // This would also, I assume, avoid any issues from other programs
    if (set_exclusive_locking_mode_db(db)
            || create_db_version_table_db(db)
            || version_db(db, src_file, &version))
    {
        rc = -1;
        goto error;
    }

    s64 new_version = version;
    for (int i = 0; i < n_versions; i++) {
        if (versions[i].version <= new_version) continue;

        rc = ddl_db(db, B(versions[i].sql));
        if (rc) goto error;

        new_version = versions[i].version;
    }

    if (new_version > version) {
        rc = set_version_db(db, src_file, new_version);
        if (rc == 0) {
            log_var_blob(db_file);
            log_var_blob(src_file);
            log_var_s64(new_version);
        }
    }

error:
    // NOTE(jason): closing the db releases the pragma locking_mode exclusive lock
    close_db(db);

    return rc;
}

