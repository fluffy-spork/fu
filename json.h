#pragma once

typedef enum {
    NONE,
    OBJECT,
    ARRAY,
    FIELD_NAME,
    FIELD_VALUE
} context_json_t;

typedef struct  {
    blob_t * blob;
    size_t offset; // into blob
    context_json_t context;
    s32 code;
} json_status_t;

// NOTE(jason): invalid json to have a trailing comma on in object or array.
// end_field and end_element_array automatically append a ','.  this removes it
// on the final element/field.  It's assumed (untested) to be marginally faster
// when generating a larger number of fields or array elements.
int
trim_trailing_comma_json(blob_t * json)
{
    if (json->size > 0 && json->data[json->size - 1] == ',') {
        json->size--;
    }

    return 0;
}

ssize_t
begin_json(blob_t * json)
{
    return write_blob(json, "{", 1);
}

ssize_t
end_json(blob_t * json)
{
    trim_trailing_comma_json(json);

    return write_blob(json, "}", 1);
}

ssize_t
begin_array_json(blob_t * json)
{
    return write_blob(json, "[", 1);
}

ssize_t
end_array_json(blob_t * json)
{
    trim_trailing_comma_json(json);

    return write_blob(json, "]", 1);
}

ssize_t
end_element_array_json(blob_t * json)
{
    return write_blob(json, ",", 1);
}

ssize_t
escape_json(blob_t * json, const blob_t * unescaped)
{
    return c_escape_blob(json, unescaped);
}

// add a blob of text that will be escaped
// TODO(jason): why does this exist?
ssize_t
add_json(blob_t * json, const blob_t * unescaped)
{
    return escape_json(json, unescaped);
}

ssize_t
add_cstr_json(blob_t * json, const char * cstr_unescaped)
{
    return escape_json(json, B(cstr_unescaped));
    //return add_cstr_blob(json, cstr_unescaped);
}

// the name should not require escaping
int
begin_field_json(blob_t * json, const blob_t * name)
{
    write_blob(json, "\"", 1);
    add_blob(json, name);
    write_blob(json, "\"", 1);

    write_blob(json, ":", 1);

    return json->error;
}

ssize_t
end_field_json(blob_t * json)
{
    return write_blob(json, ",", 1);
}

int
string_field_json(blob_t * json, const blob_t * name, const blob_t * value)
{
    assert(name != NULL);
    assert(value != NULL);

    begin_field_json(json, name);

    write_blob(json, "\"", 1);
    add_json(json, value);
    write_blob(json, "\"", 1);

    end_field_json(json);

    return 0;
}

// add preformatted value (json) as a field value by avoiding normal string escaping
int
raw_field_json(blob_t * json, blob_t * name, blob_t * raw)
{
    begin_field_json(json, name);

    add_blob(json, raw);

    end_field_json(json);

    return 0;
}

int
cstr_field_json(blob_t * json, blob_t * name, const char * cstr)
{
    begin_field_json(json, name);

    write_blob(json, "\"", 1);
    add_cstr_json(json, cstr);
    write_blob(json, "\"", 1);

    end_field_json(json);

    return 0;
}

// 2020-01-01T10:32:22.123456789Z zulu time with nanosecond precision
int
timestamp_field_json(blob_t * json, blob_t * name, struct timespec timestamp)
{
    assert(available_blob(json) >= 30);

    struct tm tm;
    gmtime_r(&timestamp.tv_sec, &tm);

    char s[256];
    int size = snprintf(s, 256, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.9ldZ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, timestamp.tv_nsec);

    begin_field_json(json, name);
    write_blob(json, "\"", 1);
    write_blob(json, s, size);
    write_blob(json, "\"", 1);
    end_field_json(json);

    return 0;
}

int
s64_field_json(blob_t * json, blob_t * name, s64 value)
{
    begin_field_json(json, name);
    add_s64_blob(json, value);
    end_field_json(json);

    return 0;
}

// var arg is name, type, value pointer
s32
parse_object_field_json(json_status_t * status, const char * name, ...)
{
    (void)name;
    status->code = 1;
    return 0;
}

