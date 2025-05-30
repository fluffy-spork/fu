#pragma once

// NOTE(jason): DO NOT CHANGE THE ORDER OF FIELDS since it will change the
// field ids
// - I don't think this is going to work.  will need to define the ids
//   explicitly as I've already messed it up once.
//
// XXX(jason): currently requires including fields used by web_fu.h or app_fu.h
// that's not good and is planned to be fixed.  likely by replacing the db_fu.h
#define FIELD_TABLE(E) \
    E(unknown, "unknown", unknown_type_field, 0, 0) \
    E(user_id, "User ID", integer_type_field, 0, 0) \
    E(last_id, "Last ID", hidden_type_field, 0, 32) \
    E(modified, "Modified", timestamp_type_field, 0, 0) \
    E(created, "Created", timestamp_type_field, 0, 0) \
    E(alias, "Alias", text_type_field, 0, 32) \
    E(email, "Email", text_type_field, 0, 50) \
    E(session_id, "Session ID", integer_type_field, 0, 0) \
    E(cookie, "Cookie", text_type_field, 0, 32) \
    E(next_id, "Next ID", hidden_type_field, 0, 255) \
    E(size, "Size", integer_type_field, 0, 0) \
    E(content_type, "Content Type", integer_type_field, 0, 0) \
    E(file_id, "Add Image or Video", file_type_field, 0, 0) \
    E(path, "path", text_type_field, 0, 256) \
    E(status, "status", integer_type_field, 0, 0) \
    E(kind, "kind", text_type_field, 0, 32) \
    E(msg, "msg", text_type_field, 0, 64) \
    E(hello_id, "hello_id", integer_type_field, 0, 0) \
    E(access_log_id, "access_log_id", integer_type_field, 0, 0) \
    E(request_method, "request_method", integer_type_field, 0, 0) \
    E(request_path, "request_path", text_type_field, 0, 1024) \
    E(http_status, "http_status", integer_type_field, 0, 0) \
    E(received, "received", integer_type_field, 0, 0) \
    E(elapsed_ns, "elapsed_ns", integer_type_field, 0, 0) \
    E(request_content_length, "request_content_length", integer_type_field, 0, 0) \
    E(response_content_length, "response_content_length", integer_type_field, 0, 0) \

