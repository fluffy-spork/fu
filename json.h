#pragma once

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


error_t
init_json(void)
{
    return 0;
}





// XXX: query_json.  should this be a different file.  feeling yes

typedef struct query_json_s query_json_t;
typedef error_t (* callback_json_f)(query_json_t * qj);

#define MAX_DEPTH_JSON 256

struct query_json_s {
    blob_t * input;
    size_t offset;

    // character of value
    u8 context[MAX_DEPTH_JSON];
    int depth;

    blob_t value;
    blob_t name;

    callback_json_f callback;
    void * data_callback;
};


blob_t
view_blob(blob_t * b, size_t offset, size_t size)
{
    return (blob_t) {
        .data = b->data + offset,
        .capacity = size,
        .size = size, 
        .error = 0,
        .constant = true
    };
}


error_t
next_token_json(query_json_t * qj, u8 token)
{
    //u8 depth = qj->depth;
    //debug_s64(depth);

    blob_t * input = qj->input;

    error_t result = 0;

//    debug_u8(token);
//    debug_s64(qj->offset);

    for_i_offset_blob(input, qj->offset) {
        u8 c = input->data[i];

//        debug_u8(c);

        if ('{' == c) {
            qj->depth++;
            qj->context[qj->depth] = c;
            qj->offset = i + 1;
        }
        else if ('}' == c) {
            qj->depth--;
            qj->offset = i + 1;
            result = token == ',';
            break;
        }
        else if ('[' == c) {
            qj->depth++;
            qj->context[qj->depth] = c;
            qj->offset = i + 1;
        }
        else if (']' == c) {
            qj->depth--;
            qj->offset = i + 1;
            result = token == ',';
            break;
        }
        else if (',' == c) { // end of container value
            u8 context = qj->context[qj->depth];
            if ('{' == context) {
                // in object
            }
            else if ('[' == context) {
                // in array
            }
            qj->offset = i + 1;
        }
        else if (':' == c) {
            qj->name = qj->value;
            qj->value.size = 0;
            qj->offset = i + 1;
        }
        else if ('"' == c) {
            u8 prev = 0;
            size_t off = i + 1;

            for_j_offset_size(input->size, off) {
                u8 d = input->data[j];
                if ('"' == d && prev != '\\') {
                    qj->value = view_blob(input, off, j - off);
                    i = j;
                    break;
                }

                prev = d;
            }
            qj->offset = i + 1;
        }
        else if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
            size_t off = i;
            for_j_offset_size(input->size, off + 1) {
                u8 d = input->data[j];
                if ((d < '0' || d > '9') && d != '.' && d != 'e' && d != 'E') {
                    qj->value = view_blob(input, off, j - off);
                    i = j - 1;
                    break;
                }
            }
            qj->offset = i + 1;
        }
        else if (c == 't') {
            debug("begin true");
            // bad
            qj->offset = i + 4;
        } else if (c == 'f') {
            debug("begin false");
            // bad
            qj->offset = i + 5;
        }
        else if (c == 'n') {
            debug("begin null");
            // bad
            qj->offset = i + 4;
        }

        if (c == token) {
            result = 1;
            break;
        }
    }

    return result;
}


error_t
next_object_json(query_json_t * qj)
{
    return next_token_json(qj, '{');
}


error_t
next_value_json(query_json_t * qj)
{
    return next_token_json(qj, ',');
}

// should be the next field within the current object
error_t
next_field_json(query_json_t * qj)
{
    if (qj->context[qj->depth] != '{') {
        return 0;
//        debugf("%c", qj->context[qj->depth]);
//        error_log("invalid context", "json", 1);
//        return -1;
    }

    return next_token_json(qj, ':');
}


error_t
next_value_names_json(query_json_t * qj, blob_t * names[], size_t size_names, blob_t values[])
{
    blob_t * name = &qj->name;

    for_i_size(size_names) {
        if (equal_blob(name, names[i]) && next_value_json(qj) > 0) {
            values[i] = qj->value;
            return 1;
        }
    }

    return 0;
}

error_t
find_array_json(query_json_t * qj)
{
    return next_token_json(qj, '[');
}


// if not currently in an object, search the tree for an object
error_t
find_token_json(query_json_t * qj, u8 token)
{
    if (qj->context[qj->depth] == token) {
        return 0;
    }

    return next_token_json(qj, '{');
}


// offset should be beginning of value in input
error_t
find_field_json(query_json_t * qj, const blob_t * name)
{
    if (find_token_json(qj, '{') == -1) {
        return -1;
    }

    u8 depth = qj->depth;
    while (next_field_json(qj) > 0 && qj->depth >= depth) {
        if (empty_blob(&qj->name)) break;
        //debug_blob(&qj->name);
        if (equal_blob(&qj->name, name)) {
            break;
        }
    }

    return 0;
}


/*
error_t
foreach_array_json(query_json_t * qj, callback_json_f callback, void * data_callback)
{
    return 0;
}
*/


error_t
debug_callback_json(query_json_t * qj)
{
    UNUSED(qj);

    debug_blob(&qj->value);

    return 0;
}


error_t
html_callback_json(query_json_t * qj)
{
    blob_t * html = qj->data_callback;
    u8 context = qj->context[qj->depth];

    switch (context) {
        case '{':
            start_div_class(B("object"));
            break;
        case '}':
            end_div();
            break;
        case '[':
            start_div_class(B("array"));
            break;
        case ']':
            end_div();
            break;
        case ':':
            escape_html(html, &qj->name);
            write_blob(html, ":", 1);
            escape_html(html, &qj->value);
            break;
        case ',':
            // end of element or field
            escape_html(html, &qj->value);
            break;
        case '"':
            escape_html(html, &qj->value);
            break;
        default:
            debugf("unknown context: %c", context);
            break;
    }

    return 0;
}

/*
error_t
parse_json(parser_json_t * parser)
{
    blob_t * input = parser->input;
    dev_error(input == NULL);

    debug_blob(input);

    int max_depth = 256;
    int depth = 0;
    u8 stack[max_depth];
    stack[0] = 0;

    for_i_blob(input) {
        u8 c = input->data[i];

        if ('"' == c) {
            u8 prev = 0;
            size_t off = i + 1;
            for (size_t j = off; j < input->size; j++) {
                u8 d = input->data[j];
                if ('"' == d && prev != '\\') {
                    // end of string
                    if (parser->string_callback) {
                        blob_t * string = wrap_size_blob(input->data + off, j - off);
                        parser->string_callback(parser, string);
                    }
                    i = j;
                    break;
                }

                prev = d;
            }
        }
        else if ('{' == c) {
            debug("begin object");
            debug("name context");
            depth++;
            stack[depth] = c;
        }
        else if ('}' == c) {
            debug("end object");
            depth--;
            debugf("%c context", stack[depth]);
        }
        else if ('[' == c) {
            debug("begin array");
            debug("element context");
            depth++;
            stack[depth] = c;
        }
        else if (']' == c) {
            debug("end array");
            depth--;
            debugf("%c context", stack[depth]);
        }
        else if (',' == c) {
            // end array element or object field
            debug("comma");
        }
        else if (':' == c) {
            debug("value context");
        }
        else if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
            size_t off = i;
            for (size_t j = off + 1; j < input->size; j++) {
                u8 d = input->data[j];
                if ((d < '0' || d > '9') && d != '.' && d != 'e' && d != 'E') {
                    if (parser->number_callback) {
                        blob_t * number = wrap_size_blob(input->data + off, j - off);
                        parser->number_callback(parser, number);
                    }
                    i = j - 1;
                    break;
                }
            }
        }
        else if (c == 't' || c == 'f') {
            debug("begin boolean");
        }
        else if (c == 'n') {
            debug("begin null");
        }
    }

    return 0;
}
*/


/*
error_t
callback_json(blob_t * input, callback_json_t callback)
{
    query_json_t qj = {
        .input = input,
        .callback = callback,
    };

    return 0;
}
*/

/*
error_t
block_json(blob_t * input, ^(query_json_t * qj))
{
    return 0;
}
*/

/*
callback_json(B("{ \"some json\" }"), debug_callback_json);
*/

